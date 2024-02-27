/*
  Library:        lcd16x2 - Parallel 8/4 bits
  Written by:     Mohamed Yaqoob
  Date Written:   04/12/2017
  Updated:        26/06/2020
  Description:    This is a library for the standard 16X2 LCD display, for the STM32 MCUs based on HAL libraries.
                  It perfroms the basic Text/Number printing to your 16X2 LCD, in 8 bits and 4 bits modes of operation.

  References**:
                  This was written based on the open source Arduino LiquidCrystal library
                  and by referring to the DATASHEET of the LCD16X2, also with the help of
                  the following YouTube tutorials on LCD 16X2:
                  (1): 'RC Tractor Guy' YouTube tutorial on the following link:
                       https://www.youtube.com/watch?v=efi2nlsvbCI
                  (2): 'Explore Embedded' YouTube tutorial on the following link:
                       https://www.youtube.com/watch?v=YDJISiPUdA8

 * Copyright (C) 2017 - M.Yaqoob - MutexEmbedded
   This is a free software under the GNU license, you can redistribute it and/or modify it under the terms
   of the GNU General Public License version 3 as published by the Free Software Foundation.

   This software library is shared with public for educational purposes, without WARRANTY and Author is not liable for any damages caused directly
   or indirectly by this software, read more about this on the GNU General Public License.
*/

#ifndef LCD16X2_H_
#define LCD16X2_H_

#include <stdbool.h>
#include <inttypes.h>

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

