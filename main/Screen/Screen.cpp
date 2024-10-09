#include "Screen.hpp"
#include "lcd_native.h"

#include <iostream>
#include <string>
#include <vector>
#include <map>

static const char *TAG = "LCDScreen";

void LCDScreen::start(void)
{
  ESP_LOGI(TAG, "LCDScreen start");
  xTaskCreate(&LCDScreen::init, "LCDScreen::init", 4 * 1024, this, 5, NULL);
}

void LCDScreen::stop(void)
{
  ESP_LOGI(TAG, "LCDScreen stop");
  xTaskCreate(&LCDScreen::deinit, "LCDScreen::deinit", 4 * 1024, this, 5, NULL);
}

void LCDScreen::init(void *arg)
{
  LCDScreen *self = static_cast<LCDScreen *>(arg);

  /*  LCD initialization */
  LCD_init(0x27, GPIO_NUM_22, GPIO_NUM_21, 20, 4);
  LCD_home();
  LCD_clearScreen();
  LCD_writeStr("----- 20x4 LCD -----");
  LCD_setCursor(0, 1);
  LCD_writeStr("LCD Library Demo");

  vTaskDelete(NULL);
}

void LCDScreen::deinit(void *arg)
{
  LCDScreen *self = static_cast<LCDScreen *>(arg);

  vTaskDelete(NULL);
}

void LCDScreen::onReceive(CentralServices s, void *data)
{
  ESP_LOGI(TAG, "LCDScreen onReceive: %d", s);
}