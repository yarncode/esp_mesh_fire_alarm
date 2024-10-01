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

class ApiCaller : public Observer
{
public:
  ApiCaller()
  {
    this->_service = CentralServices::API_CALLER;
    this->_state_create_device = false;
  };
  void onReceive(CentralServices s, void *data) override;
  void start(void);
  void stop(void);
  bool isCallingCreateDevice(void);

private:
  static void init(void *arg);
  static void deinit(void *arg);
  static void addDevice(void *arg);

  bool _state_create_device;
};
