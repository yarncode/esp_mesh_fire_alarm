#ifndef CONST_TYPE_H
#define CONST_TYPE_H

#include <string>
#include <vector>
#include <iostream>

#include "sdkconfig.h"

enum CentralServices
{
  NONE,
  ENCODER,
  STORAGE,
  MESH,
  LOG,
  MQTT,
  BLUETOOTH,
  REFACTOR,
  BUTTON,
  LED,
  API_CALLER,
  SENSOR,
  SNTP,
};

namespace common
{
  static const std::string CONFIG_PATH_FILE_WIFI_CONFIG = "/config_wifi.txt";
  static const std::string CONFIG_PATH_FILE_SERVER_CONFIG = "/config_server.txt";
#ifdef CONFIG_MODE_GATEWAY
  static const std::string CONFIG_NODE_TYPE_DEVICE = "GATEWAY";
#elif CONFIG_MODE_NODE
  static const std::string CONFIG_NODE_TYPE_DEVICE = "NODE";
#endif
}

#endif // CONST_TYPE_H
