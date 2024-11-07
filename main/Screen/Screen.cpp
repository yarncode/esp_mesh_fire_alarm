#include "Screen.hpp"
#include "lcd_native.h"
#include "Cache.h"
#include "Mqtt.hpp"

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <format>
#include <ctime>
#include <chrono>

#include "esp_mesh.h"

static const char *TAG = "LCDScreen";

using namespace std;
using namespace std::chrono;

void LCDScreen::start(void)
{
  ESP_LOGI(TAG, "LCDScreen start");
  xTaskCreateWithCaps(&LCDScreen::init, "LCDScreen::init", 4 * 1024, this, 5, NULL, MALLOC_CAP_8BIT);
}

void LCDScreen::stop(void)
{
  ESP_LOGI(TAG, "LCDScreen stop");
  xTaskCreateWithCaps(&LCDScreen::deinit, "LCDScreen::deinit", 4 * 1024, this, 5, NULL, MALLOC_CAP_8BIT);
}

void LCDScreen::clearLine(uint8_t row)
{
  char msg[16];
  memset(msg, ' ', sizeof(msg));
  LCD_setCursor(0, row);
  LCD_writeStr(msg);
}

void LCDScreen::writeByRow(uint8_t row, uint8_t col, std::string text)
{
  char msg[16];
  memset(msg, 0, sizeof(msg));

  /* clear line */
  this->clearLine(row);

  LCD_setCursor(col, row);
  memcpy(msg, text.c_str(), sizeof(msg));
  LCD_writeStr(msg);
}

void LCDScreen::init(void *arg)
{
  LCDScreen *self = static_cast<LCDScreen *>(arg);
  Mqtt *mqtt = static_cast<Mqtt *>(self->getObserver(CentralServices::MQTT));
  char msg[16];

  /*  LCD initialization */
  LCD_init(0x27, GPIO_NUM_21, GPIO_NUM_22, 16, 4);
  LCD_home();
  LCD_clearScreen();

  while (true)
  {
    /* info ip gateway */
    self->writeByRow(0, 0, "IP: " + (cacheManager.ip.length() > 0 ? cacheManager.ip : std::string("No connect")));
    /* info count node childs */
    self->writeByRow(1, 0, "Num: " + std::to_string((esp_mesh_get_routing_table_size() - 1)));
    /* info ram size */
    // sprintf(msg, "Ram: %.1f KB", (float)esp_get_free_heap_size() / 1023);
    // self->writeByRow(2, 0, std::string(msg));
    /* print server status */
    self->writeByRow(2, 0, "Server: " + std::string(mqtt->isConnected() ? "accept" : "lost"));
    /* print date time */
    time_t _t = time(nullptr);
    tm *lt = localtime(&_t);
    sprintf(msg, "%02d:%02d:%02d", lt->tm_hour, lt->tm_min, lt->tm_sec);
    self->writeByRow(3, 0, "Time: " + std::string(msg));

    vTaskDelay(5000 / portTICK_PERIOD_MS);
  }

  vTaskDeleteWithCaps(NULL);
}

void LCDScreen::deinit(void *arg)
{
  LCDScreen *self = static_cast<LCDScreen *>(arg);

  vTaskDeleteWithCaps(NULL);
}

void LCDScreen::onReceive(CentralServices s, void *data)
{
  ESP_LOGI(TAG, "LCDScreen onReceive: %d", s);
}