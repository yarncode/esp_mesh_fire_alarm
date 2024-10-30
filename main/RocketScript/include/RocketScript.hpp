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

class RocketScript : public Observer
{
public:
  RocketScript()
  {
    this->_service = CentralServices::ROCKET_SCRIPT;
  };
  void onReceive(CentralServices s, void *data) override;
  void start(void);
  void stop(void);

private:
  static void init(void *arg);
  static void deinit(void *arg);
};
