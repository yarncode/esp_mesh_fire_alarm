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

#include "esp_event.h"
#include "esp_log.h"
#include "esp_mesh.h"
#include "esp_wifi.h"
#include "esp_mac.h"

class Mesh : public Observer
{
public:
  Mesh()
  {
    this->_service = CentralServices::MESH;
    this->_instance = this;
  };
  void onReceive(CentralServices s, void *data) override;
  void start(void);
  void stop(void);
  void stopWifiAndMesh(void);
  void startWiFi(void);
  void startMesh(void);
  bool isStarted(void);

#ifdef CONFIG_MODE_GATEWAY
  void startEthernet(void);
#endif

  int layer = -1;
  int lastLayer = 0;
  mesh_addr_t meshAddress;
  mesh_addr_t routingTable[CONFIG_MESH_ROUTE_TABLE_SIZE];
  mesh_addr_t parentAddress;
  esp_ip4_addr_t currentIp;

private:
  static void eth_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
  static void mesh_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
  static void ip_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
  static void init(void *arg);
  static void deinit(void *arg);
  static void handleFactory(void *arg);
  static void callbackData(mesh_addr_t *from, mesh_data_t *data);
  static void recieveMsg(void *arg);
  static void transmitDemo(void *arg);
  void startBase(void);
  void deinitBase(void);
  void initWiFiNative(void);
  void initMeshNative(void);
  void deinitWiFiNative(void);
  void deinitMeshNative(void);
  void sendMessage(uint8_t *addr, uint8_t *data, uint16_t len);

  /* default data */
  bool _isStarted = false;
  bool _isNativeStart = false;
  bool _isParentConnected = false;
  bool _isStartBase = false;
  bool _isStartBaseWiFi = false;
  bool _isStartBaseMesh = false;
  bool _isStartEthernet = false;
  bool _isStartTCP = false;
  bool _isStartLoopDefault = false;
  bool _isStartNetIF = false;
  std::map<CentralServices, bool> _condition_state;

  static const uint8_t meshId[6];
  static const std::string meshPassword;
  /* router */
  static const std::string ssid;
  static const std::string password;

  static const std::string ssid_2;
  static const std::string password_2;

  static Mesh *_instance;
};
