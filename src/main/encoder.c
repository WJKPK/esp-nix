/*
 * Copyright 2024 WJKPK
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

#include <stdint.h>
#include <limits.h>

#include "encoder.h"
#include "encoder_fsm.h"

#include "utilities/timer.h"
#include "utilities/scheduler.h"

#include <esp_attr.h>
#include <driver/gpio.h>

#define LOGGER_OUTPUT_LEVEL LOG_OUTPUT_DEBUG
#include "utilities/logger.h"

#define ENCODER_PUSH_PIN GPIO_NUM_0
#define ENCODER_A_PIN GPIO_NUM_20
#define ENCODER_B_PIN GPIO_NUM_21

static void process_detected_state(void* args, uint32_t state) {
    encoder_event_type event = ENCODER_EVENT_LAST;
    switch (state) {
        case ENCODER_DIRECTION_CLOCKWISE:
            log_info("Up!");
            event = ENCODER_EVENT_UP;
            break;

        case ENCODER_DIRECTION_COUNTERCLOCKWISE:
            event = ENCODER_EVENT_DOWN;
            log_info("Down!");
            break;

        default:
            break;
    }
    scheduler_enqueue(SchedulerQueueMenu, &event);
}

static void process_button_click(void* args, uint32_t state) {
    log_info("Click!");
    encoder_event_type event = ENCODER_EVENT_PUSH;
    scheduler_enqueue(SchedulerQueueMenu, &event);
}


IRAM_ATTR void gpio_encoder_isr_routine(void *arg) {
    encoder_fsm_output direction = encoder_fms_process(gpio_get_level(ENCODER_A_PIN), gpio_get_level(ENCODER_B_PIN));
    switch (direction) {
        case ENCODER_DIRECTION_CLOCKWISE:
        case ENCODER_DIRECTION_COUNTERCLOCKWISE:
            timer_soft_irq(process_detected_state, NULL, (uint32_t)direction); 
            break;
        default:
            break;
    }
}

IRAM_ATTR void gpio_encoder_push_isr_routine(void *arg) {
    bool is_pressed = !gpio_get_level(ENCODER_PUSH_PIN);
    if (is_pressed)
        timer_soft_irq(process_button_click, NULL, 0); 
}

error_status_t encoder_init(void) {
    uint64_t pin_mask_isr = ((1ULL << ENCODER_PUSH_PIN)  |
                             (1ULL << ENCODER_B_PIN)     |
                             (1ULL << ENCODER_A_PIN));

    gpio_reset_pin(ENCODER_A_PIN);
	gpio_reset_pin(ENCODER_B_PIN);

    gpio_config_t io_conf_isr = {
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = 1,
        .pull_down_en = 0,
        .intr_type = GPIO_INTR_ANYEDGE,
        .pin_bit_mask = pin_mask_isr
    };

    gpio_config(&io_conf_isr);
    gpio_isr_handler_add(ENCODER_PUSH_PIN, gpio_encoder_push_isr_routine, NULL);
    gpio_isr_handler_add(ENCODER_A_PIN, gpio_encoder_isr_routine, NULL);
    gpio_isr_handler_add(ENCODER_B_PIN, gpio_encoder_isr_routine, NULL);

    return ERROR_ANY;
}

