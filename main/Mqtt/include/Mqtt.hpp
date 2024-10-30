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

#include "string.h"
#include <esp_event.h>

#include "mqtt_client.h"

enum ChannelMqtt
{
  CHANNEL_DATA,
  CHANNEL_CONTROL,
  CHANNEL_NOTIFY,
  CHANNEL_CONFIG,
  CHANNEL_SENSOR,
  CHANNEL_ACTIVE,
};

class Mqtt : public Observer
{
public:
  Mqtt()
  {
    this->_service = CentralServices::MQTT;
  };
  void onReceive(CentralServices s, void *data) override;
  void start(void);
  void stop(void);
  bool isStarted(void);
  int sendMessage(ChannelMqtt chanel, std::string msg);

private:
  void onConnected(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);
  void onDisconnected(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);
  static void init(void *arg);
  static void deinit(void *arg);
  static void notifySyncThresholdTrigger(void *arg);
  static void notifyDeviceCreated(void *arg);
  static void recieveMsg(void *arg);
  static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);

  bool _isStarted = false;
  bool _isConnected = false;
};
