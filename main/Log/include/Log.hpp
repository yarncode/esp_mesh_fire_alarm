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

#include "esp_log.h"

class Log : public Observer
{
public:
  Log()
  {
    this->_service = CentralServices::LOG;
  };
  void onReceive(CentralServices s, void *data) override;
  void start(void);

private:
  static void init(void *arg);
  uint32_t heapSize = 0;
};
