#ifndef CACHE_MANGAGER_H
#define CACHE_MANGAGER_H

#include <string>
#include <vector>
#include <iostream>
#include <map>
#include <cstdint>

#include "ConstType.h"
#include "EventType.hpp"

class Cache
{
public:
  Cache()
  {
  }
  /* wifi config */
  std::string activeToken;
  std::string mainToken;
  std::string ssid;
  std::string password;
  std::string ip;
  std::string gateway;
  std::string netmask;
  std::uint8_t sta_bssid[6];
  std::uint8_t device_mac[6];
  bool bssid_set;
  bool history_station_connected;

  /* server config */
  std::string serverHost;
  std::string m_username;
  std::string m_password;
  std::string scheme;
  std::string serverIp;
  std::string protocol;
  int serverPort;

  /* threshold trigger */
  std::vector<int> thresholds_temp;
  std::vector<int> thresholds_humi;
  std::vector<int> thresholds_smoke;
};

extern Cache cacheManager;

#endif
