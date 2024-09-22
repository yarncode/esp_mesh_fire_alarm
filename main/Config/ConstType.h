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
};

namespace common
{
  static const std::string CONFIG_PATH_FILE_WIFI_CONFIG = "/config_wifi.txt";
  static const std::string CONFIG_PATH_FILE_SERVER_CONFIG = "/config_server.txt";
}

#endif // CONST_TYPE_H
