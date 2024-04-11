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
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include "heat_controller_interface.h"
#include "heat_controller.h"
#include "ble.h"
#include "menu.h"
#include "utilities/error.h"
#include "utilities/scheduler.h"
#include "utilities/addons.h"
#include "utilities/types.h"

#define LOGGER_OUTPUT_LEVEL LOG_OUTPUT_DEBUG
#include "utilities/logger.h"

typedef enum {
    WRITE_ANY         = 0x0,
    WRITE_TEMPERATURE = 0x1,
    WRITE_TIME        = 0x2,
    WRITE_BOTH        = 0x3
} write_map;
_Static_assert((WRITE_TIME | WRITE_TEMPERATURE) == WRITE_BOTH, "");
_Static_assert(WRITE_ANY == 0, "");

typedef enum {
    COMPONENT_IDLE_PRIORITY,
    COMPONENT_MENU_PRIORITY,
    COMPONENT_BLE_PRIORITY,
    COMPONENT_LAST_PRIORITY
} components_priority;

static struct {
    heating_request_type request_type;
    struct {
        celcius   temperature;
        seconds   time;
        write_map map;
    }                   const_temperature_settings;
    components_priority processed_request;
} ctx;

static bool is_request_priority_higher_than_proccesed(components_priority incoming_priority) {
    return incoming_priority > ctx.processed_request;
}

static read_buff_descriptor_t on_ble_read(simplified_uuid_t uuid) {
    static uint16_t last_readout = 0;

    last_readout = heat_controller_get_temperature();

    read_buff_descriptor_t last_readout_descriptor = {
        .buff = &last_readout,
        .size = sizeof(last_readout),
    };
    read_buff_descriptor_t const_temperature_descriptor = {
        .buff = &ctx.const_temperature_settings.temperature,
        .size = sizeof(ctx.const_temperature_settings.temperature)
    };
    read_buff_descriptor_t const_time_descriptor = {
        .buff = &ctx.const_temperature_settings.time,
        .size = sizeof(ctx.const_temperature_settings.time)
    };
    read_buff_descriptor_t mode_descriptor = {
        .buff = &ctx.request_type,
        .size = sizeof(ctx.request_type)
    };

    switch (uuid) {
        case HEATER_MODE_WRITE_UUID:
            return mode_descriptor;

        case HEATER_TEMPERATURE_READ_UUID:
            return last_readout_descriptor;

        case HEATER_CONST_TEMPERATURE_WRITE_UUID:
            return const_temperature_descriptor;

        case HEATER_CONST_TIME_WRITE_UUID:
            return const_time_descriptor;

        default:
            break;
    }
    return EMPTY_READ_BUFF;
} /* on_ble_read */

static multistage_heating_type map_request_to_multistage_type(heating_request_type type) {
    multistage_heating_type map[HEATING_REQUEST_LAST] = {
        [HEATING_REQUEST_CONSTANT] = MULTISTAGE_HEATING_LAST,
        [HEATING_REQUEST_JEDEC]    = MULTISTAGE_HEATING_JEDEC
    };

    return map[type];
}

static void unblock_menu_operations(void) {
    menu_event_type event = MENU_EVENT_PREEMPT_TAKE;

    scheduler_enqueue(SchedulerQueueMenu, &event);
    ctx.processed_request = COMPONENT_IDLE_PRIORITY;
}

static void block_menu_operations(void) {
    menu_event_type event = MENU_EVENT_PREEMPT_REQUEST;

    scheduler_enqueue(SchedulerQueueMenu, &event);
}

static error_status_t request_mode_via_ble(heating_request_type mode) {
    error_status_t result = ERROR_INVALID_INPUT_PARAMETER;

    block_menu_operations();
    (void) heat_controller_cancel_action();
    ctx.request_type = mode;

    switch (mode) {
        case HEATING_REQUEST_CONSTANT:
            if (ctx.const_temperature_settings.map != WRITE_BOTH)
                return ERROR_INVALID_STATE;

            result = heat_controller_start_constant_heating(ctx.const_temperature_settings.temperature,
                ctx.const_temperature_settings.time, unblock_menu_operations);
            break;

        case HEATING_REQUEST_LAST:
        case HEATING_REQUEST_CANCEL:
            break;

        default:
            result = heat_controller_start_multistage_heating_mode(map_request_to_multistage_type(mode),
                unblock_menu_operations);
            break;
    }
    if (result != ERROR_ANY) {
        ctx.request_type = HEATING_REQUEST_LAST;
        error_print_message(result);
    }
    ble_notify(HEATER_MODE_WRITE_UUID);
    return result;
}

static void on_ble_request(simplified_uuid_t uuid, uint8_t* buff, size_t size) {
    heating_request_type mode = *((heating_request_type*) buff);

    if (!is_request_priority_higher_than_proccesed(COMPONENT_BLE_PRIORITY))
        return;

    ctx.processed_request = COMPONENT_BLE_PRIORITY;

    switch (uuid) {
        case HEATER_CONST_TEMPERATURE_WRITE_UUID:
            ctx.const_temperature_settings.map |= WRITE_TEMPERATURE;
            memcpy(&ctx.const_temperature_settings.temperature, buff,
              sizeof(ctx.const_temperature_settings.temperature));
            break;

        case HEATER_CONST_TIME_WRITE_UUID:
            ctx.const_temperature_settings.map |= WRITE_TIME;
            memcpy(&ctx.const_temperature_settings.time, buff, sizeof(ctx.const_temperature_settings.time));
            break;

        case HEATER_MODE_WRITE_UUID:
            if (mode >= HEATING_REQUEST_LAST)
                return;

            if (mode == HEATING_REQUEST_CANCEL) {
                ctx.request_type = mode;
                heat_controller_cancel_action();
                break;
            }

            if (ERROR_ANY == request_mode_via_ble(mode)) {
                if (mode == HEATING_REQUEST_CONSTANT)
                    memset(&ctx.const_temperature_settings, 0, sizeof(ctx.const_temperature_settings));
            }
            break;

        default:
            return;
    }
    ble_notify(uuid);
} /* on_ble_request */

static void inform_about_job_done(void) {
    menu_event_type event = MENU_EVENT_REQUEST_DONE;

    scheduler_enqueue(SchedulerQueueMenu, &event);
    ctx.processed_request = COMPONENT_IDLE_PRIORITY;
}

static void on_menu_request(void* _request) {
    error_status_t result   = ERROR_LAST;
    heater_request* request = _request;

    if (!is_request_priority_higher_than_proccesed(COMPONENT_MENU_PRIORITY))
        return;

    ctx.processed_request = COMPONENT_MENU_PRIORITY;
    ctx.request_type      = request->type;

    switch (request->type) {
        case HEATING_REQUEST_CONSTANT:
            ctx.const_temperature_settings.temperature = request->constant.const_temperature;
            ctx.const_temperature_settings.time        = request->constant.duration;
            ble_notify(HEATER_CONST_TEMPERATURE_WRITE_UUID);
            ble_notify(HEATER_CONST_TIME_WRITE_UUID);

            result = heat_controller_start_constant_heating(request->constant.const_temperature,
                request->constant.duration, inform_about_job_done);
            break;

        case HEATING_REQUEST_JEDEC:
            result = heat_controller_start_multistage_heating_mode(
                map_request_to_multistage_type(request->type), inform_about_job_done);
            break;

        case HEATING_REQUEST_CANCEL:
            result = heat_controller_cancel_action();
            break;

        default:
            break;
    }
    if (result != ERROR_ANY) {
        ctx.request_type = HEATING_REQUEST_LAST;
        error_print_message(result);
    }
    ble_notify(HEATER_MODE_WRITE_UUID);
} /* on_menu_request */

error_status_t heat_controller_interface_init(void) {
    ctx.request_type = HEATING_REQUEST_LAST;
    static const simplified_uuid_t read_uuid_filter[] = { HEATER_MODE_WRITE_UUID,
                                                          HEATER_TEMPERATURE_READ_UUID,
                                                          HEATER_CONST_TEMPERATURE_WRITE_UUID,
                                                          HEATER_CONST_TIME_WRITE_UUID };
    read_observer_descriptor_t read_observer_descriptor = {
        .observer     = on_ble_read,
        .uuid_filter  = read_uuid_filter,
        .filter_count = COUNT_OF(read_uuid_filter),
    };

    static const simplified_uuid_t write_uuid_filter[] = { HEATER_MODE_WRITE_UUID,
                                                           HEATER_CONST_TEMPERATURE_WRITE_UUID,
                                                           HEATER_CONST_TIME_WRITE_UUID };
    write_observer_descriptor_t write_observer_descriptor = {
        .observer     = on_ble_request,
        .uuid_filter  = write_uuid_filter,
        .filter_count = COUNT_OF(write_uuid_filter),
    };

    scheduler_subscribe(SchedulerQueueHeatControlerInterface, on_menu_request);
    error_status_t result = ERROR_ANY;
    if (ERROR_ANY != (result = ble_add_read_observer(read_observer_descriptor)))
        return result;

    return ble_add_write_observer(write_observer_descriptor);
}
