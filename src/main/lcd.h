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

#ifndef _MAIN_LCD_
#define _MAIN_LCD_

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
    struct {
        unsigned start_position;
        char content[LCD_MAX_LINE_LEN];
    } text;
    struct {
        unsigned start_position;
        custom_symbol symbol;
    } symbol;
} lcd_line;

typedef struct {
    lcd_line line_descriptors[LCD_MAX_LINE_NUMBER];
} lcd_descriptor;

#define LCD_EMPTY_LINE ((lcd_line){ .text.content = "", .symbol.symbol = kCustomSymbolLast})
#define LCD_EMPTY_SCREEN ((lcd_descriptor){ .line_descriptors[0] = LCD_EMPTY_LINE, .line_descriptors[1] = LCD_EMPTY_LINE })

error_status_t ldc_init(void);
error_status_t lcd_send_request(lcd_descriptor* lcd_descriptor);

#endif // _MAIN_LCD_

