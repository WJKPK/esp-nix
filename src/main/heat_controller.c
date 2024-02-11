/*
 * Copyright 2023 WJKPK
 *
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
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
#include "ble.h"
#include "heater_calculator.h"
#include "utilities/addons.h"

typedef unsigned celcius;
const unsigned invalid_stage_index = UINT_MAX;

static uint16_t last_readout = 0;

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

void on_temperature_read(simplified_uuid_t uuid, uint8_t** buff, size_t* size) {
    switch (uuid) {
        case HEATER_TEMPERATURE_READ_UUID: {
            *buff = (uint8_t*) &last_readout;
            *size = sizeof(last_readout);
            break;
        }
        default:
            break;
    }
}

typedef enum {
    HeatingModeJedec,
    HeatingModeLast 
} heating_mode_type;

typedef struct {
    struct {
        seconds from;
        seconds to;
    } time;
    celcius temperature;
} temperature_stage;

typedef struct {
    seconds time;
    temperature_stage* stages;
    unsigned actual;
    unsigned count;
} heating_mode_descriptor;

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
    for (i = heating_mode->actual; i < heating_mode->count; i++) {
        is_time_in_range(heating_mode->stages[i], heating_mode->time) ? ({ is_found = true; break; }) : ({});
    }
    heating_mode->actual = i;
    if (!is_found)
        heating_mode->actual = invalid_stage_index;
}

celcius get_actual_setpoint(heating_mode_descriptor* heating_mode) {
    return heating_mode->stages[heating_mode->actual].temperature;
}

static void execute_heating_mode_periodic(void* heating_mode);
static void execute_heating_mode(heating_mode_descriptor* heating_mode, miliseconds actual_period_length) {
    if (error_any != spi_read(SpiDeviceThermocoupleAfe, &last_readout, sizeof(last_readout))) {
            set_toggler_level(false);
            return;
    }
    set_actual_stage(heating_mode);
    if (heating_mode->actual == invalid_stage_index) {
        timer_unregister_callback(heat_controller_tick, execute_heating_mode_periodic);
        return;
    }
    float percent = get_heating_power_percent((float)last_readout, get_actual_setpoint(heating_mode));
    log_debug("time: %u, temperature read: %u, power set to: %f%%, stage: %u",
            heating_mode->time, last_readout, percent, heating_mode->actual);
    set_toggler_level(true);
    heating_mode->time += miliseconds_to_seconds(actual_period_length);
    miliseconds turnoff_timeout = percent * actual_period_length;
    if (turnoff_timeout)
        oneshot_arm(oneshot_heater_controller, turnoff_timeout, turn_off_heater, NULL);
}

static void execute_heating_mode_periodic(void* heating_mode) {
    miliseconds time = periodic_get_period(heat_controller_tick);
    execute_heating_mode(heating_mode, time);
}

void start_given_heating_mode(heating_mode_descriptor* heating_mode) {
    miliseconds time = periodic_get_expire(heat_controller_tick);
    execute_heating_mode(heating_mode, time);
    timer_register_callback(heat_controller_tick, execute_heating_mode_periodic, heating_mode);
}

void on_heater_mode_write(simplified_uuid_t uuid, uint8_t* buff, size_t size) {
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
    }};

    static heating_mode_descriptor heating_mode[HeatingModeLast] = {
        {.stages = jedec, .count = COUNT_OF(jedec)}
    };
    switch (uuid) {
        case HEATER_MODE_WRITE_UUID:
            start_given_heating_mode(&heating_mode[HeatingModeJedec]);
            break;
        default:
            break;
    }
}

error_status_t heat_controller_init(void) {
    static const simplified_uuid_t read_uuid_filter[]   = { HEATER_TEMPERATURE_READ_UUID };
    read_observer_descriptor_t read_observer_descriptor = {
        .observer     = on_temperature_read,
        .uuid_filter  = read_uuid_filter,
        .filter_count = 1,
    };

    static const simplified_uuid_t write_uuid_filter[]   = { HEATER_MODE_WRITE_UUID };
    write_observer_descriptor_t write_observer_descriptor = {
        .observer = on_heater_mode_write,
        .uuid_filter = write_uuid_filter,
        .filter_count = 1,
    };

    return error_is_success(setup_toggler_pin())
        && error_is_success(spi_read(SpiDeviceThermocoupleAfe, &last_readout, sizeof(last_readout)))
        && error_is_success(ble_add_read_observer(read_observer_descriptor))
        && error_is_success(ble_add_write_observer(write_observer_descriptor)) ? error_any : error_library_error;
}

