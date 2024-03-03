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

#define LOGGER_OUTPUT_LEVEL LOG_OUTPUT_INFO

#include "lcd.h"
#include <rom/ets_sys.h>
#include <driver/gpio.h>
#include <string.h>
#include "FreeRTOS.h"
#include "timers.h"

#include "lcd1602/lcd1602.h"
#include "utilities/timer.h"
#include "utilities/addons.h"
#include "utilities/logger.h"
#include "utilities/scheduler.h"

#define LCD_D4_LINE 9U
#define LCD_D5_LINE 6U
#define LCD_D6_LINE 3U
#define LCD_D7_LINE 2U
#define LCD_ENABLE_LINE 4UL
#define LCD_RS_LINE 1UL

static void lcd_delay(microseconds microsec) {
    miliseconds milisec = microseconds_to_miliseconds(microsec);
    if (portTICK_PERIOD_MS > milisec) {
        ets_delay_us(microsec);
        return;
    }
    vTaskDelay(pdMS_TO_TICKS(milisec));
}

static void lcd_gpio_init(void) {
    uint64_t pin_mask = ((1ULL << LCD_D4_LINE)  |
                         (1ULL << LCD_D5_LINE)  |
                         (1ULL << LCD_D6_LINE)  |
                         (1ULL << LCD_D7_LINE)  |
                         (1ULL << LCD_RS_LINE)  |
                         (1ULL << LCD_ENABLE_LINE));
 
    gpio_config_t io_conf = {
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = 0,
        .pull_down_en = 1,
        .intr_type = GPIO_INTR_DISABLE,
        .pin_bit_mask = pin_mask
    };
    gpio_config(&io_conf);
}

bool lcd_pin_set(pin_t gpio_num, bool level) {
    return ESP_OK == gpio_set_level(gpio_num, (uint32_t)level);
}

static bool is_string(char* text) {
   return strlen(text) > 0;
}

static bool is_symbol(custom_symbol symbol) {
   return symbol != kCustomSymbolLast;
}

static void print_screen(lcd_descriptor screen) {
    for (unsigned i = 0; i < COUNT_OF(screen.line_descriptors); i++) {
        lcd_line line = screen.line_descriptors[i];
        if (is_string(line.text.content)) {
            lcd16x2_setCursor(i, line.text.start_position);
            lcd16x2_printf("%s", line.text.content);
        }
        if (is_symbol(line.symbol.symbol)) {
            lcd16x2_setCursor(i, line.symbol.start_position);
            lcd16x2_writeCustom(line.symbol.symbol);
        }
    }
}

static void on_lcd_descriptor(void* data) {
    lcd_descriptor* desc = data;
    print_screen(*desc);
}

error_status_t ldc_init(void) {
    lcd_gpio_init();
    lcd1602_interface iface = {
        .wait = lcd_delay,
        .pins = {
            .d4 = LCD_D4_LINE,
            .d5 = LCD_D5_LINE,
            .d6 = LCD_D6_LINE,
            .d7 = LCD_D7_LINE,
            .rs = LCD_RS_LINE,
            .enable = LCD_ENABLE_LINE
        },
        .pin_set = lcd_pin_set 
    };
    lcd16x2_init_4bits(iface);
    uint8_t heart[] = {
        0b00000,
        0b01010,
        0b11111,
        0b11111,
        0b01110,
        0b00100,
        0b00000,
        0b00000
    };
    uint8_t plate_program[] = {
        0b00000,
        0b01110,
        0b01010,
        0b01110,
        0b01000,
        0b01000,
        0b00000,
        0b11111
    };
    uint8_t arrow_up[] = {
        0b00000,
        0b00100,
        0b01110,
        0b11111,
        0b00100,
        0b00100,
        0b00100,
        0b00000
    };

    uint8_t arrow_down[] = {
      0b00000,
      0b00100,
      0b00100,
      0b00100,
      0b11111,
      0b01110,
      0b00100,
      0b00000
    };

    if (false == lcd16x2_createChar(kCustomSymbolHeart, heart)
        && lcd16x2_createChar(kCustomSymbolPlateProgram, plate_program)
        && lcd16x2_createChar(kCustomSymbolArrowUp, arrow_up)
        && lcd16x2_createChar(kCustomSymbolArrowDown, arrow_down))
        return error_unknown_resource;

    lcd16x2_clear();
    lcd16x2_cursorShow(false);

    lcd_descriptor start_lcd = 
    {
        .line_descriptors[0] = {
            .text = {
                .content = "ThermoPlate",
                .start_position = 2,
            },
            .symbol = {
                .symbol = kCustomSymbolArrowUp,
                .start_position = 15,
            },
        },
        .line_descriptors[1] = LCD_EMPTY_LINE
    };
    start_lcd.line_descriptors[1].symbol.symbol = kCustomSymbolArrowDown;
    start_lcd.line_descriptors[1].symbol.start_position = 15;
    print_screen(start_lcd);
    if (!scheduler_subscribe(SchedulerQueueLcd, on_lcd_descriptor))
        return error_collection_full;
    return error_any;
}

error_status_t lcd_send_request(lcd_descriptor* lcd_descriptor) {
    if (lcd_descriptor == NULL)
        return error_null;

    if (!scheduler_enqueue(SchedulerQueueLcd, lcd_descriptor))
        return error_collection_full;

    return error_any;
}

