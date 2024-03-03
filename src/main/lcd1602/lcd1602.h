/*
 * Copyright 2024 WJKPK
 *
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership. The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License. You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied. See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#ifndef LCD16X2_H_
#define LCD16X2_H_

#include <stdbool.h>
#include <inttypes.h>

#define LCD1602_MAX_LINE_LEN 16U

typedef unsigned pin_t;
typedef struct {
    pin_t rs;
    pin_t enable;
    pin_t d4;
    pin_t d5;
    pin_t d6;
    pin_t d7;
} pinset;

typedef bool (*lcd1602_pin_set)(pin_t pin, bool state);
typedef void (*lcd1602_wait)(unsigned microseconds);

typedef struct {
    pinset pins;
    lcd1602_pin_set pin_set;
    lcd1602_wait wait;
} lcd1602_interface;

void lcd16x2_init_4bits(lcd1602_interface iface);
void lcd16x2_setCursor(uint8_t row, uint8_t col);
void lcd16x2_1stLine(void);
void lcd16x2_2ndLine(void);
void lcd16x2_cursorShow(bool state);
void lcd16x2_clear(void);
void lcd16x2_display(bool state);
void lcd16x2_shiftRight(uint8_t offset);
void lcd16x2_shiftLeft(uint8_t offset);
bool lcd16x2_printf(const char* str, ...);
bool lcd16x2_createChar(uint8_t location, uint8_t charmap[]);
void lcd16x2_writeCustom(uint8_t location);

#endif /* LCD16X2_H_ */

