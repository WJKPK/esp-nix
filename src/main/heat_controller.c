/*
 * Copyright 2023 WJKPK
 *
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
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
#define LOGGER_OUTPUT_LEVEL LOG_OUTPUT_INFO

#include "heat_controller.h"

#include <stdint.h>
#include <driver/gpio.h>
#include "pid.h"
#include "spi.h"
#include "ble.h"
#include "heater_calculator.h"
#include "utilities/logger.h"
#include "utilities/addons.h"

const char* tag = "HEATER_CONTROLLER";
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

static void turn_off_heater(void) {
    set_toggler_level(false);
}

static void calculate_power(void) {
    float setpoint = 50;
    if (error_any != spi_read(SpiDeviceThermocoupleAfe, &last_readout, sizeof(last_readout))) {
            set_toggler_level(false);
            return;
    }
    float percent = get_heating_power_percent((float)last_readout, setpoint);
    log_debug(tag, "actual temperature: %u: power set to: %f", last_readout, percent);
    set_toggler_level(true);
    const unsigned ms_in_five_seconds = 5000;
    oneshot_arm(oneshot_heater_controller, percent * ms_in_five_seconds, turn_off_heater);
}

void on_heater_mode_write(simplified_uuid_t uuid, uint8_t* buff, size_t size) {
    switch (uuid) {
        case HEATER_MODE_WRITE_UUID:
            timer_register_callback(periodic_timer_five_sec, calculate_power);
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

