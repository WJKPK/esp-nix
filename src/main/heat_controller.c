/*
 * Copyright 2023 WJKPK
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more contributor license agreements.  See the NOTICE file distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include "utilities/timer.h"

#define LOGGER_OUTPUT_LEVEL LOG_OUTPUT_DEBUG
#include "utilities/logger.h"

#include "heat_controller.h"

#include <stdint.h>
#include <limits.h>
#include <driver/gpio.h>
#include "pid.h"
#include "spi.h"
#include "heater_calculator.h"
#include "utilities/addons.h"

const unsigned invalid_stage_index = UINT_MAX;

typedef struct {
    struct {
        seconds from;
        seconds to;
    } time;
    celcius temperature;
} temperature_stage;

typedef struct {
    seconds duration;
    temperature_stage* stages;
    unsigned actual_stage;
    unsigned stage_count;
    heat_completion_marker completed_routine;
} heating_mode_descriptor;

static struct {
    heating_mode_state state;
    uint16_t last_readout;
} ctx;

unsigned heat_controller_get_temperature(void) {
    return ctx.last_readout;
}

static temperature_stage jedec[] = {{
        .time = {
            .from = 0,
            .to = 100,
        },
        .temperature = 100
    }, {
        .time = {
            .from = 100,
            .to = 140,
        },
        .temperature = 250
    }, {
        .time = {
            .from = 140,
            .to = 230
        },
        .temperature = 30
    }
};

static heating_mode_descriptor heating_mode[multistage_heating_last] = {
    {.stages = jedec, .stage_count = COUNT_OF(jedec)}
};

error_status_t setup_toggler_pin(void) {
    const unsigned toggler_pin = GPIO_NUM_8;
    const gpio_config_t toggler_config = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = 1ULL << toggler_pin,
        .pull_down_en = 1,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_OK == gpio_config(&toggler_config) ?
        ({ return error_any; }) : ({ return error_unknown_resource; });
}

static error_status_t set_toggler_level(bool on) {
    ESP_OK == gpio_set_level(GPIO_NUM_8, on) ? ({ return error_any; }) : ({ return error_resource_unavailable; });
}

static void turn_off_heater(void* args) {
    set_toggler_level(false);
}

static bool is_time_in_range(temperature_stage stage, seconds time) {
    if (stage.time.from <= time && stage.time.to > time)
        return true;
    return false;
}

static void set_actual_stage(heating_mode_descriptor* heating_mode) {
    unsigned i = 0;
    bool is_found = false;
    for (i = heating_mode->actual_stage; i < heating_mode->stage_count; i++) {
        is_time_in_range(heating_mode->stages[i], heating_mode->duration) ? ({ is_found = true; break; }) : ({});
    }
    heating_mode->actual_stage = i;
    if (!is_found)
        heating_mode->actual_stage = invalid_stage_index;
}

celcius get_actual_setpoint(heating_mode_descriptor* heating_mode) {
    return heating_mode->stages[heating_mode->actual_stage].temperature;
}

static error_status_t execute_heating_mode(heating_mode_descriptor* heating_mode, miliseconds actual_period_length) {
    if (error_any != spi_read(SpiDeviceThermocoupleAfe, &ctx.last_readout, sizeof(ctx.last_readout))) {
            set_toggler_level(false);
            return error_communication_error;
    }
    set_actual_stage(heating_mode);
    if (heating_mode->actual_stage == invalid_stage_index) {
        heating_mode->completed_routine();
        heating_mode->completed_routine = NULL;
        heating_mode->actual_stage = 0;
        ctx.state = heating_state_idle;
        return error_execution_stopped;
    }
    float percent = get_heating_power_percent((float)ctx.last_readout, get_actual_setpoint(heating_mode));
    log_debug("time: %u, temperature read: %u, power set to: %f%%, stage: %u",
            heating_mode->duration, ctx.last_readout, percent, heating_mode->actual_stage);
    set_toggler_level(true);
    heating_mode->duration += miliseconds_to_seconds(actual_period_length);
    miliseconds turnoff_timeout = percent * actual_period_length;
    if (turnoff_timeout)
        oneshot_arm(oneshot_heater_controller, turnoff_timeout, turn_off_heater, NULL);

    return error_any;
}

static void execute_heating_mode_periodic(void* heating_mode) {
    if (ctx.state == heating_state_idle) 
        return;

    miliseconds time = periodic_get_period(heat_controller_tick);
    if (error_any != execute_heating_mode(heating_mode, time))
        timer_unregister_callback(heat_controller_tick, execute_heating_mode_periodic);
}

error_status_t heat_controller_start_multistage_heating_mode(multistage_heating_type type, heat_completion_marker completion_routine) {
    if (type >= multistage_heating_last)
        return error_invalid_input_parameter;

    if (ctx.state != heating_state_idle) 
        return error_invalid_state;

    heating_mode_descriptor* selected_heating_mode = &heating_mode[type];
    selected_heating_mode->duration = 0;
    selected_heating_mode->completed_routine = completion_routine; 
    ctx.state = heating_state_multi_stage;

    miliseconds time = periodic_get_expire(heat_controller_tick);
    error_status_t result = error_any;
    if (error_any != (result = execute_heating_mode(selected_heating_mode, time)))
        return result;

    return timer_register_callback(heat_controller_tick, execute_heating_mode_periodic, selected_heating_mode);
}

error_status_t heat_controller_start_constant_heating(celcius temperature, unsigned duration, heat_completion_marker completion_routine) {
    if (ctx.state != heating_state_idle) 
        return error_invalid_state;

    static temperature_stage constant = {
        .time = {
            .from = 0,
            .to = 0,
        },
        .temperature = 0
    };
    static heating_mode_descriptor constant_heating_mode = { .stages = &constant, .stage_count = 1 };

    constant.time.to = duration;
    constant.temperature = temperature;
    constant_heating_mode.duration = 0;
    constant_heating_mode.completed_routine = completion_routine;

    ctx.state = heating_state_constant;
    miliseconds time = periodic_get_expire(heat_controller_tick);
    error_status_t result = error_any;
    if (error_any != (result = execute_heating_mode(&constant_heating_mode, time)))
        return result;

    return timer_register_callback(heat_controller_tick, execute_heating_mode_periodic, &constant_heating_mode);
}

error_status_t heat_controller_cancel_action(void) {
    error_status_t status = timer_unregister_callback(heat_controller_tick, execute_heating_mode_periodic);
    if (status != error_any)
        return status;

    ctx.state = heating_state_idle;
    set_toggler_level(false);
    return status;
}

error_status_t heat_controller_init(void) {
    error_status_t result = error_any;
    if (error_any != (result = setup_toggler_pin()))
        return result;

    return spi_read(SpiDeviceThermocoupleAfe, &ctx.last_readout, sizeof(ctx.last_readout));
}

