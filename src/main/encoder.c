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

#include "encoder.h"

#include <esp_attr.h>
#include <driver/gpio.h>

#define ENCODER_PUSH_PIN GPIO_NUM_0
#define ENCODER_A_PIN GPIO_NUM_20
#define ENCODER_B_PIN GPIO_NUM_21

typedef enum {
     ENCODER_STATE_WAITING,
     ENCODER_STATE_A_RECEIVED,
     ENCODER_STATE_B_RECEIVED,
     ENCODER_STATE_UP,
     ENCODER_STATE_DOWN,
     ENCODER_STATE_INVALID
} encoder_invalidator_state;

typedef encoder_invalidator_state
(*invalidator_state_handler)(encoder_invalidator_state);

static encoder_invalidator_state
handler_waiting_state(encoder_invalidator_state input) {
     if (input == ENCODER_STATE_A_RECEIVED)
         return ENCODER_STATE_A_RECEIVED;

     if (input == ENCODER_STATE_B_RECEIVED)
         return ENCODER_STATE_B_RECEIVED;

     return ENCODER_STATE_INVALID;
}

static encoder_invalidator_state
handle_a_received(encoder_invalidator_state input) {
     if (input == ENCODER_STATE_B_RECEIVED)
         return ENCODER_STATE_UP;

     return ENCODER_STATE_INVALID;
}

static encoder_invalidator_state
handler_b_received(encoder_invalidator_state input) {
     if (input == ENCODER_STATE_A_RECEIVED)
         return ENCODER_STATE_DOWN;

     return ENCODER_STATE_INVALID;
}

const invalidator_state_handler state_table[] = {
     [ENCODER_STATE_WAITING] = handler_waiting_state,
     [ENCODER_STATE_A_RECEIVED] = handle_a_received,
     [ENCODER_STATE_B_RECEIVED] = handler_b_received,
     [ENCODER_STATE_UP] = handler_waiting_state,
     [ENCODER_STATE_DOWN] = handler_waiting_state,
     [ENCODER_STATE_INVALID] = handler_waiting_state
};

static encoder_invalidator_state
encoder_invalidator_process(encoder_invalidator_state input) {
     static encoder_invalidator_state current_state = ENCODER_STATE_WAITING;
     current_state = state_table[current_state](input);
     return current_state;
}

IRAM_ATTR void gpio_encoder_isr_routine(void *arg) {
    encoder_invalidator_state state =
        encoder_invalidator_process((encoder_invalidator_state)arg);

    switch (state) {
        case ENCODER_STATE_UP:
            //TODO implement
            break;
        case ENCODER_STATE_DOWN:
            //TODO implement
            break;
        default:
            break;
    }
}

IRAM_ATTR void gpio_encoder_push_isr_routine(void *arg) {
    //TODO implement
}

error_status_t encoder_init(void) {
    uint64_t pin_mask = ((1ULL << ENCODER_PUSH_PIN)  |
                         (1ULL << ENCODER_B_PIN) |
                         (1ULL << ENCODER_A_PIN));
 
    gpio_config_t io_conf = {
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = 1,
        .pull_down_en = 0,
        .intr_type = GPIO_INTR_NEGEDGE,
        .pin_bit_mask = pin_mask
    };
    gpio_config(&io_conf);
    gpio_isr_handler_add(ENCODER_PUSH_PIN, gpio_encoder_push_isr_routine, NULL);
    gpio_isr_handler_add(ENCODER_A_PIN, gpio_encoder_isr_routine, (void*)ENCODER_STATE_A_RECEIVED);
    gpio_isr_handler_add(ENCODER_B_PIN, gpio_encoder_isr_routine, (void*)ENCODER_STATE_B_RECEIVED);
    return error_any;
}

