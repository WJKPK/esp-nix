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
#include "menu.h"
#include "utilities/scheduler.h"
#include "utilities/timer.h"

#define LOGGER_OUTPUT_LEVEL LOG_OUTPUT_DEBUG
#include "utilities/logger.h"

typedef enum {
     MENU_STATE_INIT,
     MENU_STATE_PREEMPTED,
     MENU_STATE_HEATING_CONSTANT,
     MENU_STATE_HEATING_CONSTANT_TEMPERATURE_SET,
     MENU_STATE_HEATING_CONSTANT_TIME_SET,
     MENU_STATE_HEATING_JEDEC,
     MENU_STATE_WAIT,
     MENU_STATE_DONE,
} menu_state;

static menu_state current_state = MENU_STATE_INIT;
static unsigned const_temperature = 0;
static seconds const_time = 0;

typedef menu_state
(*invalidator_state_handler)(menu_event_type);

static void show_const_time_setup(void) {
    const char time_str[] = "time:";

    lcd_send_clean();
    lcd_send_request(0, 0, "Set");
    lcd_send_request(1, 0, time_str); 
    lcd_send_request(1, sizeof(time_str), "%u", const_time); 
}

static void show_const_temperature_setup(void) {
    const char temperature_str[] = "temperature:";

    lcd_send_clean();
    lcd_send_request(0, 0, "Set");
    lcd_send_request(1, 0, temperature_str); 
    lcd_send_request(1, sizeof(temperature_str), "%u", const_temperature); 
}

static void show_heating_constant_display(void) {
    lcd_send_clean();
    lcd_send_request(0, 0, "Constant");
    lcd_send_request(1, 0, "temperature");
}

static void show_jedec_display(void) {
    lcd_send_clean();
    lcd_send_request(0,0, "JEDEC");
}

static void show_running_state(void) {
    lcd_send_clean();
    lcd_send_request(0, 0, "Running...");
}

static void show_done_state(void) {
    lcd_send_clean();
    lcd_send_request(0, 0, "Done");
}

static void show_preempt_display(void) {
    lcd_send_clean();
    lcd_send_request(0, 0, "BLE control");
}

static void send_predefined_heating_request(void) {
    heater_request request = {
        .type = heating_request_jedec,
    };
    scheduler_enqueue(SchedulerQueueHeatControlerInterface, &request);
}

static void send_constant_heating_request(celcius temperature, seconds time) {
    heater_request request = {
        .type = heating_request_constant,
        .constant = {
            .duration = time,
            .const_temperature = temperature
        }
    };
    scheduler_enqueue(SchedulerQueueHeatControlerInterface, &request);
}

static menu_state
handle_idle_state(menu_event_type event) {
    switch (event) {
        case MENU_EVENT_ENCODER_PUSH:
            return MENU_STATE_HEATING_CONSTANT;

        case MENU_EVENT_PREEMPT_REQUEST:
            return MENU_STATE_PREEMPTED; 

        default:
            break;
    }
    return MENU_STATE_INIT;
}


static menu_state
handle_preempted_state(menu_event_type event) {
    switch (event) {
        case MENU_EVENT_PREEMPT_TAKE:
            return MENU_STATE_HEATING_CONSTANT;

        case MENU_EVENT_PREEMPT_REQUEST:
            return MENU_STATE_PREEMPTED; 

        default:
            break;
    }
    return MENU_STATE_PREEMPTED;
}


static menu_state handle_push_for_heating(void) {
    switch (current_state) {
        case MENU_STATE_HEATING_CONSTANT:
            show_const_temperature_setup();
            return MENU_STATE_HEATING_CONSTANT_TEMPERATURE_SET;

        case MENU_STATE_HEATING_JEDEC:
            send_predefined_heating_request();
            return MENU_STATE_WAIT;

        default:
            break;
    }
    return current_state;
}

static menu_state
handle_heating_state(menu_event_type event) {
    switch (event) {
        case MENU_EVENT_ENCODER_UP:
            return MENU_STATE_HEATING_CONSTANT;

        case MENU_EVENT_ENCODER_DOWN:
                return MENU_STATE_HEATING_JEDEC;

        case MENU_EVENT_ENCODER_PUSH:
            return handle_push_for_heating();

        case MENU_EVENT_PREEMPT_REQUEST:
            return MENU_STATE_PREEMPTED; 

        default:
            break;
    }
    return current_state;
}

static menu_state handle_push_for_temperature_set(void) {
    switch (current_state) {
        case MENU_STATE_HEATING_CONSTANT_TEMPERATURE_SET:
            show_const_time_setup();
            return MENU_STATE_HEATING_CONSTANT_TIME_SET;

        default:
            break;
    } return current_state;
}

static menu_state
handle_temperature_set(menu_event_type event) {
    switch (event) {
        case MENU_EVENT_ENCODER_UP:
            const_temperature++;
            show_const_temperature_setup();
            return current_state;

        case MENU_EVENT_ENCODER_DOWN:
            const_temperature == 0 ? ({}) : ({ const_temperature--;});
            show_const_temperature_setup();
            return current_state;

        case MENU_EVENT_ENCODER_PUSH:
            return handle_push_for_temperature_set();

        case MENU_EVENT_PREEMPT_REQUEST:
            return MENU_STATE_PREEMPTED; 

        default:
            break;
    }
    return current_state;
}

static menu_state
handle_time_set(menu_event_type event) {
    switch (event) {
        case MENU_EVENT_ENCODER_UP:
            const_time++;
            show_const_time_setup();
            return current_state;

        case MENU_EVENT_ENCODER_DOWN:
            const_time == 0 ? ({}) : ({ const_time--;});
            show_const_time_setup();
            return current_state;

        case MENU_EVENT_ENCODER_PUSH:
            send_constant_heating_request(const_temperature, const_time);
            return MENU_STATE_WAIT;

        case MENU_EVENT_PREEMPT_REQUEST:
            return MENU_STATE_PREEMPTED; 

        default:
            break;
    }
    return current_state;
}

static menu_state
handle_wait_state(menu_event_type event) {
    if (event == MENU_EVENT_REQUEST_DONE)
        return MENU_STATE_DONE;
    return current_state;
}

static void idle_display_show(void) {
    lcd_send_request(0,3, "ThermoPlate");
}

typedef void (*menu_drawing_callback)(void);
const struct {
    invalidator_state_handler state_handler;
    menu_drawing_callback drawing;
    } state_table[] = {
     [MENU_STATE_INIT] = {handle_idle_state, idle_display_show},
     [MENU_STATE_PREEMPTED] = {handle_preempted_state, show_preempt_display},
     [MENU_STATE_HEATING_CONSTANT] = {handle_heating_state, show_heating_constant_display},
     [MENU_STATE_HEATING_CONSTANT_TEMPERATURE_SET] = {handle_temperature_set, NULL},
     [MENU_STATE_HEATING_CONSTANT_TIME_SET] = {handle_time_set, NULL},
     [MENU_STATE_HEATING_JEDEC] = {handle_heating_state, show_jedec_display},
     [MENU_STATE_WAIT] = {handle_wait_state, show_running_state},
     [MENU_STATE_DONE] = {handle_idle_state, show_done_state},
};

static menu_state
menu_process(menu_state input) {
     current_state = state_table[current_state].state_handler(input);

     if (NULL != state_table[current_state].drawing)
        state_table[current_state].drawing();

     return current_state;
}

void handle_incoming_requests(void* args) {
    menu_event_type* event = args;
    menu_process(*event);
}

void menu_init(void) {
    idle_display_show();
    current_state = MENU_STATE_INIT;
    scheduler_subscribe(SchedulerQueueMenu, handle_incoming_requests);
}

