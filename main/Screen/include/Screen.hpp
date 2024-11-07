#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <map>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"

#include "Observer.h"
#include "ConstType.h"
#include "EventType.hpp"

class LCDScreen : public Observer
{
public:
  LCDScreen()
  {
    this->_service = CentralServices::LCD;
  };
  void clearLine(uint8_t row);
  void writeByRow(uint8_t row, uint8_t col, std::string text);
  void onReceive(CentralServices s, void *data) override;
  void start(void);
  void stop(void);

private:
  static void init(void *arg);
  static void deinit(void *arg);

  int count_childs;

};
