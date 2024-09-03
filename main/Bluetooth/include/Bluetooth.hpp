#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <map>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <nlohmann/json.hpp>

#include "Observer.h"
#include "ConstType.h"
#include "EventType.hpp"

using json = nlohmann::json;

class Bluetooth : public Observer, public BLEServerCallbacks, public BLECharacteristicCallbacks
{
public:
  Bluetooth()
  {
    this->_service = CentralServices::BLUETOOTH;
  };
  void onReceive(CentralServices s, void *data) override;
  void start(void);
  void stop(void);

private:
  static void init(void *arg);
  static void deinit(void *arg);
  static void saveWifiAndStartMesh(void *arg);
  virtual void onConnect(BLEServer *server);
  virtual void onDisconnect(BLEServer *server);
  virtual void onWrite(BLECharacteristic *characteristic);
  virtual void onRead(BLECharacteristic *characteristic);

  std::string _rawData;
};
