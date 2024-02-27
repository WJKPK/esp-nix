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

// r/w always low
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#include "lcd1602.h"

static lcd1602_interface interface;

//Milisecond function
#define LCD_MS_DELAY(X) interface.wait((X) * 1000)

/* List of COMMANDS */
#define LCD_CLEARDISPLAY      0x01
#define LCD_RETURNHOME        0x02
#define LCD_ENTRYMODESET      0x04
#define LCD_DISPLAYCONTROL    0x08
#define LCD_CURSORSHIFT       0x10
#define LCD_FUNCTIONSET       0x20
#define LCD_SETCGRAMADDR      0x40
#define LCD_SETDDRAMADDR      0x80

#define LCD_DISPLAY_B         0x01
#define LCD_DISPLAY_C         0x02
#define LCD_DISPLAY_D         0x04

#define LCD_FUNCTION_F        0x04
#define LCD_2LINES            0x08
#define LCD_FUNCTION_DL       0x10

#define LCD_DISPLAYMOVE      0x08
#define LCD_CURSORMOVE       0x00
#define LCD_MOVERIGHT        0x04
#define LCD_MOVELEFT         0x00

/* LCD Library Variables */
static uint8_t DisplayControl = 0x0F;
static uint8_t FunctionSet = 0x00;

static void lcd16x2_enablePulse(void) {
    interface.pin_set(interface.pins.enable, true);
    interface.wait(20); 
    interface.pin_set(interface.pins.enable, false);
    interface.wait(60);
}

static void lcd16x2_rs(bool state) {
    interface.pin_set(interface.pins.rs, state);
}

static void lcd16x2_write(uint8_t wbyte) {
    uint8_t LSB_nibble = wbyte&0xF, MSB_nibble = (wbyte>>4)&0xF;
    //MSB data
    interface.pin_set(interface.pins.d4, (bool)(MSB_nibble&0x1));
    interface.pin_set(interface.pins.d5, (bool)(MSB_nibble&0x2));
    interface.pin_set(interface.pins.d6, (bool)(MSB_nibble&0x4));
    interface.pin_set(interface.pins.d7, (bool)(MSB_nibble&0x8));
    lcd16x2_enablePulse();
    //LSB data
    interface.pin_set(interface.pins.d4, (bool)(LSB_nibble&0x1));
    interface.pin_set(interface.pins.d5, (bool)(LSB_nibble&0x2));
    interface.pin_set(interface.pins.d6, (bool)(LSB_nibble&0x4));
    interface.pin_set(interface.pins.d7, (bool)(LSB_nibble&0x8));
    lcd16x2_enablePulse();
}

static void lcd16x2_writeCommand(uint8_t cmd) {
    lcd16x2_rs(false);
    lcd16x2_write(cmd);
}

static void lcd16x2_writeData(uint8_t data) {
    lcd16x2_rs(true);
    lcd16x2_write(data);
}

void lcd16x2_writeCustom(uint8_t location) {
     location &= 0x7; // we only have 8 locations 0-7
     lcd16x2_writeData(location);
}

static void lcd16x2_write4(uint8_t nib) {
    nib &= 0xF;
    lcd16x2_rs(false);
    //LSB data
    interface.pin_set(interface.pins.d4, (bool)(nib&0x1));
    interface.pin_set(interface.pins.d5, (bool)(nib&0x2));
    interface.pin_set(interface.pins.d6, (bool)(nib&0x4));
    interface.pin_set(interface.pins.d7, (bool)(nib&0x8));
    lcd16x2_enablePulse();
}

void lcd16x2_init_4bits(lcd1602_interface iface) {
    interface = iface;
    LCD_MS_DELAY(50);
    //Set GPIO Ports and Pins data
    FunctionSet = 0x28; //FUNCTIONSET & DISPLAYCONTROL??
    lcd16x2_write4(0x3);
    LCD_MS_DELAY(5);
    lcd16x2_write4(0x3);
    LCD_MS_DELAY(1);
    lcd16x2_write4(0x3);
    LCD_MS_DELAY(1);
    lcd16x2_write4(0x2);  //4 bit mode
    LCD_MS_DELAY(1);
    //4. Function set; Enable 2 lines, Data length to 4 bits
    lcd16x2_writeCommand(LCD_FUNCTIONSET | LCD_2LINES);
    //3. Display control (Display ON, Cursor ON, blink cursor)
    lcd16x2_writeCommand(LCD_DISPLAYCONTROL | LCD_DISPLAY_B | LCD_DISPLAY_C | LCD_DISPLAY_D);
    //4. Clear LCD and return home
    lcd16x2_writeCommand(LCD_CLEARDISPLAY);
    LCD_MS_DELAY(3);
}

bool lcd16x2_createChar(uint8_t location, uint8_t charmap[]) {
    if (location > 7)
        return false;
    location &= 0x7; // we only have 8 locations 0-7
    lcd16x2_writeCommand(LCD_SETCGRAMADDR | (location << 3));
    for (int i=0; i<8; i++) {
        lcd16x2_writeData(charmap[i]);
    }
    return true;
}

void lcd16x2_setCursor(uint8_t row, uint8_t col) {
  uint8_t maskData;
  maskData = col & 0x0F;
  if(row == 0) {
    maskData |= (0x80);
    lcd16x2_writeCommand(maskData);
  } else {
    maskData |= (0xc0);
    lcd16x2_writeCommand(maskData);
  }
}

void lcd16x2_1stLine(void) {
    lcd16x2_setCursor(0,0);
}

void lcd16x2_2ndLine(void) {
    lcd16x2_setCursor(1,0);
}

void lcd16x2_cursorShow(bool state) {
    if(state) {
        DisplayControl |= (LCD_DISPLAY_B | LCD_DISPLAY_C);
    }
    else {
        DisplayControl &= ~(LCD_DISPLAY_B | LCD_DISPLAY_C);
    }
    lcd16x2_writeCommand(DisplayControl);
}

void lcd16x2_clear(void) {
    lcd16x2_writeCommand(LCD_CLEARDISPLAY);
    LCD_MS_DELAY(10);
}

void lcd16x2_display(bool state) {
    if(state) {
        DisplayControl |= (LCD_DISPLAY_D);
    }
    else {
        DisplayControl &= ~(LCD_DISPLAY_D);
    }
    lcd16x2_writeCommand(DisplayControl);
}

void lcd16x2_shiftRight(uint8_t offset) {
    for(uint8_t i = 0; i < offset; i++) {
        lcd16x2_writeCommand(LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVERIGHT);
    }
}

void lcd16x2_shiftLeft(uint8_t offset) {
    for(uint8_t i = 0; i < offset; i++) {
        lcd16x2_writeCommand(LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVELEFT);
    }
}

bool lcd16x2_printf(const char* str, ...) {
    char string_array[16];
    int string_size = sizeof(string_array);

    va_list args;
    va_start(args, str);
    int result = snprintf(string_array, string_size, str, args);
    va_end(args);
    
    if (result >= string_size || result < 0)
        return false;
    
    for(uint8_t i = 0;  i < strlen(string_array); i++) {
        lcd16x2_writeData((uint8_t)string_array[i]);
    }
    return true;
}

