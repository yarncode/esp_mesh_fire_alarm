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

  enum LoggerEventType
  {
    EVENT_LOGGER_START,
    EVENT_LOGGER_STOP,
  };

  enum MqttEventType
  {
    EVENT_MQTT_START,
    EVENT_MQTT_STOP,
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

  enum LedType
  {
    EVENT_LED_UPDATE_MODE,
  };

  enum ApiCallerType
  {
    EVENT_API_CALLER_START,
    EVENT_API_CALLER_STOP,
    EVENT_API_CALLER_ADD_DEVICE,
  };

  enum SensorType
  {
    SENSOR_START,
    SENSOR_STOP,
    SENSOR_SEND_SAMPLE_DATA,
  };
}

namespace ServicePayload
{

  template <typename EventType = nullptr_t, typename DataType = nullptr_t>
  class RecievePayload_2
  {
  public:
    RecievePayload_2(EventType type, DataType data) : type(type), data(data) {}

    EventType type;
    DataType data;
  };

}
