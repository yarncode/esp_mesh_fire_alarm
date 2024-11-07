#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <map>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "Observer.h"
#include "ConstType.h"
#include "EventType.hpp"

enum FNMode {
  FN_NONE,
  FN_CONFIG,
};

class Button : public Observer
{
public:
  Button()
  {
    this->_service = CentralServices::BUTTON;
  };
  void onReceive(CentralServices s, void *data) override;
  void start(void);
  void stop(void);

private:
  static void init(void *arg);
  static void deinit(void *arg);
  void handleFNMode(uint8_t mode);
  static void button_db_click_cb(void *arg, void *data);
};
