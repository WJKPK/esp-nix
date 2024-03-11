/*
 * Copyright 2024 WJKPK
 *  
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "encoder.h"
#include "lcd.h"
#include "utilities/scheduler.h"

#define LOGGER_OUTPUT_LEVEL LOG_OUTPUT_DEBUG
#include "utilities/logger.h"

typedef enum {
     MENU_STATE_INIT,
     MENU_STATE_HEATING_CONSTANT,
     MENU_STATE_HEATING_CONSTANT_TEMPERATURE_SET,
     MENU_STATE_HEATING_JEDEC,
     MENU_STATE_WAIT,
     MENU_STATE_DONE,
} menu_state;

static menu_state current_state = MENU_STATE_INIT;
static unsigned const_temperature = 0;
const char temperature_str[] = "temperature:";

typedef menu_state
(*invalidator_state_handler)(encoder_event_type);

static menu_state
handle_idle_state(encoder_event_type event) {
    switch (event) {
        case ENCODER_EVENT_PUSH:
            lcd_send_clean();
            lcd_send_request(0, 0, "Constant");
            lcd_send_request(1, 0, "temperature");
            return MENU_STATE_HEATING_CONSTANT;

        default:
            break;
    }
    return MENU_STATE_INIT;
}

static menu_state handle_push_for_heating(void) {
    switch (current_state) {
        case MENU_STATE_HEATING_CONSTANT:
            lcd_send_clean();
            lcd_send_request(0, 0, "Set");
            lcd_send_request(1, 0, "%s", temperature_str); 
            return MENU_STATE_HEATING_CONSTANT_TEMPERATURE_SET;

        default:
            break;
    }
    return current_state;
}

static menu_state
handle_heating_state(encoder_event_type event) {
    switch (event) {
        case ENCODER_EVENT_UP:
            lcd_send_clean();
            lcd_send_request(0, 0, "Constant");
            lcd_send_request(1, 0, "temperature");
            return MENU_STATE_HEATING_CONSTANT;

        case ENCODER_EVENT_DOWN:
            lcd_send_clean();
            lcd_send_request(0,0, "JEDEC");
            return MENU_STATE_HEATING_JEDEC;

        case ENCODER_EVENT_PUSH:
            return handle_push_for_heating();

        default:
            break;
    }
    return current_state;
}

static menu_state handle_push_for_temperature_set(void) {
    switch (current_state) {
        case MENU_STATE_HEATING_CONSTANT_TEMPERATURE_SET:
            lcd_send_clean();
            lcd_send_request(0, 0, "Running...");
            return MENU_STATE_WAIT;

        default:
            break;
    } return current_state;
}

static void RefreshConstTemperatureSettup(void) {
    lcd_send_clean();
    lcd_send_request(0, 0, "Set");
    lcd_send_request(1, 0, temperature_str); 
    lcd_send_request(1, sizeof(temperature_str), "%u", const_temperature); 
}

static menu_state
handle_temperature_set(encoder_event_type event) {
    switch (event) {
        case ENCODER_EVENT_UP:
            const_temperature++;
            RefreshConstTemperatureSettup();
            return current_state;

        case ENCODER_EVENT_DOWN:
            const_temperature == 0 ? ({}) : ({ const_temperature--;});
            RefreshConstTemperatureSettup();
            return current_state;

        case ENCODER_EVENT_PUSH:
            return handle_push_for_temperature_set();

        default:
            break;
    }
    return current_state;
}

static menu_state
handle_wait_state(encoder_event_type event) {
    return MENU_STATE_DONE;
}

const invalidator_state_handler state_table[] = {
     [MENU_STATE_INIT] = handle_idle_state,
     [MENU_STATE_HEATING_CONSTANT] = handle_heating_state,
     [MENU_STATE_HEATING_CONSTANT_TEMPERATURE_SET] = handle_temperature_set,
     [MENU_STATE_HEATING_JEDEC] = handle_heating_state,
     [MENU_STATE_WAIT] = handle_wait_state,
     [MENU_STATE_DONE] = handle_idle_state,
};

static menu_state
menu_process(menu_state input) {
     current_state = state_table[current_state](input);
     return current_state;
}

void handle_incoming_requests(void* args) {
    encoder_event_type* event = args;
    menu_process(*event);
}

void menu_init(void) {
    lcd_send_request(0,3, "ThermoPlate");

    current_state = MENU_STATE_INIT;
    scheduler_subscribe(SchedulerQueueMenu, handle_incoming_requests);
}

