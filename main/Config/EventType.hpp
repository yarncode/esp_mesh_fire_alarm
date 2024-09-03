#pragma once

#include <string>
#include <vector>
#include <iostream>
#include <map>

namespace ServiceType
{
  enum TemplateEventType
  {
    EVENT_TEMPLATE_START,
    EVENT_TEMPLATE_STOP,
  };

  enum StorageEventType
  {
    EVENT_STORAGE_STARTED,
    EVENT_STORAGE_STOP,
    EVENT_STORAGE_UPDATE_INFO_WIFI,
  };

  enum MeshEventType
  {
    EVENT_MESH_START,
    EVENT_MESH_STOP,
  };

  enum BleEventType
  {
    EVENT_BLE_START,
    EVENT_BLE_STOP,
  };

  enum RefactorType
  {
    REFACTOR_ALL,
    REFACTOR_WIFI,
    REFACTOR_MQTT,
  };

  enum ButtonType
  {
    EVENT_BUTTON_CONFIG,
  };
}

namespace ServicePayload
{
  template <typename EventType = nullptr_t>
  class RecievePayload
  {
  public:
    RecievePayload(EventType type, std::map<std::string, std::string> data) : type(type), data(data) {}

    EventType type;
    std::map<std::string, std::string> data;
  };
}
