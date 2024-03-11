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
#include <stdarg.h>
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

error_status_t lcd_send_request_custom(unsigned line, unsigned position, custom_symbol symbol) {
    lcd_request request = {
        .commanand = lcd_command_print_symbol,
        .position = position,
        .line = line,
        .symbol = symbol
    };

    if (!scheduler_enqueue(SchedulerQueueLcd, &request))
        return error_collection_full;

    return error_any;
}

error_status_t lcd_send_request_string(unsigned line, unsigned position, const char* str, ...) {
    lcd_request request = {
        .commanand = lcd_command_print_string,
        .position = position,
        .line = line,
    };

    va_list args;
    va_start(args, str);
    int result = vsnprintf(request.text, LCD_MAX_LINE_LEN, str, args);
    va_end(args);
    
    if (result >= LCD_MAX_LINE_LEN || result < 0)
        return error_resource_unavailable;

    if (!scheduler_enqueue(SchedulerQueueLcd, &request))
        return error_collection_full;

    return error_any;
}

error_status_t lcd_send_clean(void) {
    lcd_request request = {
        .commanand = lcd_command_clean
    };

    if (!scheduler_enqueue(SchedulerQueueLcd, &request))
        return error_collection_full;

    return error_any;

}

static void print_screen(lcd_request* request) {
    switch(request->commanand) {
        case lcd_command_clean:
            lcd16x2_clear();
            break;

        case lcd_command_print_symbol:
            lcd16x2_setCursor(request->line, request->position);
            lcd16x2_writeCustom(request->symbol);
            break;

        case lcd_command_print_string:
            lcd16x2_setCursor(request->line, request->position);
            lcd16x2_printf("%s", request->text);
            break;

        default:
            break;
    }
}

static void on_lcd_request(void* data) {
    lcd_request* lcd_req = data;
    print_screen(lcd_req);
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

    if (false == (lcd16x2_createChar(kCustomSymbolHeart, heart)
        && lcd16x2_createChar(kCustomSymbolPlateProgram, plate_program)
        && lcd16x2_createChar(kCustomSymbolArrowUp, arrow_up)
        && lcd16x2_createChar(kCustomSymbolArrowDown, arrow_down)))
        return error_unknown_resource;

    lcd16x2_cursorShow(false);

    if (!scheduler_subscribe(SchedulerQueueLcd, on_lcd_request))
        return error_collection_full;
    return error_any;
}


