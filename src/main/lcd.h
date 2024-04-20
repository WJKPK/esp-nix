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

#ifndef _MAIN_LCD_
#define _MAIN_LCD_

#ifdef __cplusplus
extern "C" {
#endif

#include "utilities/error.h"

#define LCD_MAX_LINE_LEN 16U
#define LCD_MAX_LINE_NUMBER 2U

typedef enum {
    kCustomSymbolHeart,
    kCustomSymbolPlateProgram,
    kCustomSymbolArrowUp,    
    kCustomSymbolArrowDown,    
    kCustomSymbolLast
} custom_symbol;

typedef struct {
    enum {
        lcd_command_print_symbol,
        lcd_command_print_string,
        lcd_command_clean
    } commanand;
    unsigned line;
    unsigned position;
    union {
        char text[LCD_MAX_LINE_LEN];
        custom_symbol symbol;
    };
} lcd_request;

error_status_t ldc_init(void);
error_status_t lcd_send_clean(void);

error_status_t lcd_send_request_custom(unsigned line, unsigned position, custom_symbol symbol);
error_status_t lcd_send_request_string(unsigned line, unsigned position, const char* str, ...);
#define lcd_send_request(line, position, arg, ...) \
    _Generic((arg), \
        custom_symbol: lcd_send_request_custom, \
        const char*: lcd_send_request_string, \
        char*: lcd_send_request_string \
    )(line, position, arg, ##__VA_ARGS__)

#ifdef __cplusplus
}
#endif

#endif // _MAIN_LCD_

