#ifndef LCD_NATIVE_H
#define LCD_NATIVE_H

#include "driver/i2c_master.h"
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <stdio.h>
#include "rom/ets_sys.h"
#include <esp_log.h>

#ifdef __cplusplus
extern "C"
{
#endif

    void LCD_init(uint8_t addr, uint8_t dataPin, uint8_t clockPin, uint8_t cols, uint8_t rows);
    void LCD_setCursor(uint8_t col, uint8_t row);
    void LCD_home(void);
    void LCD_clearScreen(void);
    void LCD_writeChar(char c);
    void LCD_writeStr(char *str);

#ifdef __cplusplus
}
#endif

#endif