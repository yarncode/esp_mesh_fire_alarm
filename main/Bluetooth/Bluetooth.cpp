#include "Bluetooth.hpp"

#include "Mesh.hpp"
#include "Mqtt.hpp"
#include "Led.hpp"

#include <iostream>
#include <string>
#include <vector>
#include <map>

#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID_CONFIG "1423492b-2bc3-4761-b2ba-8988573698a9"

using namespace ServicePayload;
using namespace ServiceType;

static const char *TAG = "Bluetooth";

void Bluetooth::onConnect(BLEServer *server)
{
  ESP_LOGI(TAG, "Bluetooth onConnect");
  server->getAdvertising()->stop();
}

void Bluetooth::onDisconnect(BLEServer *server)
{
  ESP_LOGI(TAG, "Bluetooth onDisconnect");
  server->startAdvertising(); // restart advertising
}

void Bluetooth::onWrite(BLECharacteristic *characteristic)
{
  ESP_LOGI(TAG, "Bluetooth onWrite");
  try
  {
    String rxValue = characteristic->getValue();
    String UUID = characteristic->getUUID().toString();

    this->_rawData = rxValue.c_str();

    xTaskCreate(this->saveWifiAndStartMesh, "Bluetooth::saveWifiAndStartMesh", 6 * 1024, this, 5, NULL);
  }
  catch (const std::exception &e)
  {
    std::cerr << e.what() << '\n';
  }
}

void Bluetooth::saveWifiAndStartMesh(void *arg)
{
  Bluetooth *self = static_cast<Bluetooth *>(arg);

  try
  {
    json j = json::parse(self->_rawData.c_str());

    std::map<std::string, std::string> payload;
    payload["ssid"] = j["ssid"].get<std::string>();
    payload["password"] = j["password"].get<std::string>();
    payload["token"] = j["token"].get<std::string>();

    /* save info wifi to storage */
    self->notify(self->_service, CentralServices::STORAGE, new RecievePayload_2<StorageEventType, std::map<std::string, std::string>>(EVENT_STORAGE_UPDATE_INFO_WIFI, payload));

    /* stop service bluetooth */
    self->stop();
    vTaskDelay(3000 / portTICK_PERIOD_MS);

    /* start mesh service */
    self->notify(self->_service, CentralServices::MESH, new RecievePayload_2<MeshEventType, nullptr_t>(ServiceType::EVENT_MESH_START, nullptr));

#ifdef CONFIG_MODE_GATEWAY

    self->notify(self->_service, CentralServices::API_CALLER, new RecievePayload_2<ApiCallerType, nullptr_t>(ApiCallerType::EVENT_API_CALLER_ADD_DEVICE, nullptr));

#endif
  }
  catch (const std::exception &e)
  {
    std::cerr << e.what() << '\n';
  }

  vTaskDelete(NULL);
}

void Bluetooth::onRead(BLECharacteristic *characteristic)
{
  ESP_LOGI(TAG, "Bluetooth onRead");

  String UUID = characteristic->getUUID().toString();

  ESP_LOGI(TAG, "UUID: %s", UUID.c_str());
}

void Bluetooth::start(void)
{
  ESP_LOGI(TAG, "Bluetooth start");
  xTaskCreateWithCaps(&Bluetooth::init, "Bluetooth::init", 4 * 1024, this, 4, NULL, MALLOC_CAP_SPIRAM);
}

void Bluetooth::stop(void)
{
  ESP_LOGI(TAG, "Bluetooth stop");
  xTaskCreateWithCaps(&Bluetooth::deinit, "Bluetooth::deinit", 4 * 1024, this, 5, NULL, MALLOC_CAP_SPIRAM);
}

void Bluetooth::init(void *arg)
{
  Bluetooth *self = static_cast<Bluetooth *>(arg);

  ESP_LOGI(TAG, "Bluetooth init");

  /* check service mesh is running */
  Mesh *mesh = static_cast<Mesh *>(self->getObserver(CentralServices::MESH));
  Mqtt *mqtt = static_cast<Mqtt *>(self->getObserver(CentralServices::MQTT));

  /* if mqtt is running -> request to stop service */
  if (mqtt->isStarted())
  {
    mqtt->stop();
  }

  /* if mesh is running -> request to stop service */
  if (mesh->isStarted())
  {
    mesh->stopWifiAndMesh();
  }

  if (BLEDevice::getInitialized() == false)
  {
    BLEDevice::init("Fire Alarm");

    BLEServer *pServer = BLEDevice::createServer();
    pServer->setCallbacks(self);

    // Create the BLE Service
    BLEService *pServiceConfig = pServer->createService(SERVICE_UUID);

    // Create a BLE Characteristic
    BLECharacteristic *pCharacteristicConfig = pServiceConfig->createCharacteristic(
        CHARACTERISTIC_UUID_CONFIG,
        BLECharacteristic::PROPERTY_READ |
            BLECharacteristic::PROPERTY_WRITE);

    pCharacteristicConfig->setCallbacks(self);

    // Start the service
    pServiceConfig->start();

    // Create a BLE Advertising discription
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06); // functions that help with iPhone connections issue
    pAdvertising->setMinPreferred(0x12);

    // Start advertising
    BLEDevice::startAdvertising();

    /* start led status */
    self->notify(self->_service, CentralServices::LED, new RecievePayload_2<LedType, led_custume_mode_t>(EVENT_LED_UPDATE_MODE, LED_LOOP_DUP_SIGNAL));
  }

  vTaskDeleteWithCaps(NULL);
}

void Bluetooth::deinit(void *arg)
{
  Bluetooth *self = static_cast<Bluetooth *>(arg);

  if (BLEDevice::getInitialized())
  {
    BLEDevice::stopAdvertising();
    BLEDevice::deinit();

    /* stop led status */
    self->notify(self->_service, CentralServices::LED, new RecievePayload_2<LedType, led_custume_mode_t>(EVENT_LED_UPDATE_MODE, LED_MODE_NONE));
  }

  vTaskDeleteWithCaps(NULL);
}

void Bluetooth::onReceive(CentralServices s, void *data)
{
  ESP_LOGI(TAG, "Bluetooth onReceive: %d", s);
  switch (s)
  {
  case CentralServices::BUTTON:
  {
    RecievePayload_2<ButtonType, nullptr_t> *payload = static_cast<RecievePayload_2<ButtonType, nullptr_t> *>(data);
    if (payload->type == EVENT_BUTTON_CONFIG)
    {
      if (BLEDevice::getInitialized() == false)
      {
        /* start ble service */
        this->start();
      }
      else
      {
        /* stop ble service */
        this->stop();
      }
    }
    delete payload;
    break;
  }

  default:
    break;
  }
}