/*
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

#include "encoder.h"
#include <esp_attr.h>
#include <driver/gpio.h>

#define ENCODER_PUSH_PIN GPIO_NUM_0
#define ENCODER_LEFT_PIN GPIO_NUM_20
#define ENCODER_RIGHT_PIN GPIO_NUM_21

typedef enum {
    encoder_state_none,
    encoder_state_pre_up,
    encoder_state_pre_down,
    encoder_state_up,
    encoder_state_down,
    encoder_state_last
} encoder_state;

IRAM_ATTR void gpio_encoder_isr_routine(void *arg) {
    encoder_state* state = arg;
}

IRAM_ATTR void gpio_encoder_push_isr_routine(void *arg) {
}

error_status_t encoder_init(void) {
    uint64_t pin_mask = ((1ULL << ENCODER_PUSH_PIN)  |
                         (1ULL << ENCODER_RIGHT_PIN) |
                         (1ULL << ENCODER_LEFT_PIN));
 
    gpio_config_t io_conf = {
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = 1,
        .pull_down_en = 0,
        .intr_type = GPIO_INTR_NEGEDGE,
        .pin_bit_mask = pin_mask
    };
    gpio_config(&io_conf);
    gpio_isr_handler_add(ENCODER_PUSH_PIN, gpio_encoder_push_isr_routine, NULL);
    gpio_isr_handler_add(ENCODER_LEFT_PIN, gpio_encoder_isr_routine, NULL);
    gpio_isr_handler_add(ENCODER_RIGHT_PIN, gpio_encoder_isr_routine, NULL);
    return error_any;
}

