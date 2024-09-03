#include "Bluetooth.hpp"

#include "Mesh.hpp"
#include "Mqtt.hpp"

#include <iostream>
#include <string>
#include <vector>
#include <map>

#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID_CONFIG "1423492b-2bc3-4761-b2ba-8988573698a9"

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

    xTaskCreate(this->saveWifiAndStartMesh, "Bluetooth::saveWifiAndStartMesh", 4 * 1024, this, 5, NULL);
  }
  catch (const std::exception &e)
  {
    std::cerr << e.what() << '\n';
  }
}

void Bluetooth::saveWifiAndStartMesh(void *arg)
{
  Bluetooth *self = static_cast<Bluetooth *>(arg);

  json j = json::parse(self->_rawData.c_str());

  std::map<std::string, std::string> payload;
  payload["ssid"] = j["ssid"].get<std::string>();
  payload["password"] = j["password"].get<std::string>();

  /* save info wifi to storage */
  self->notify(self->_service, CentralServices::STORAGE, new ServicePayload::RecievePayload<ServiceType::StorageEventType>(ServiceType::EVENT_STORAGE_UPDATE_INFO_WIFI, payload));

  /* start mesh service */
  self->notify(self->_service, CentralServices::MESH, new ServicePayload::RecievePayload<ServiceType::MeshEventType>(ServiceType::EVENT_MESH_START, {}));

  /* stop service bluetooth */
  self->stop();

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
  xTaskCreate(&Bluetooth::init, "Bluetooth::init", 4 * 1024, this, 5, NULL);
}

void Bluetooth::stop(void)
{
  ESP_LOGI(TAG, "Bluetooth stop");
  xTaskCreate(&Bluetooth::deinit, "Bluetooth::deinit", 4 * 1024, this, 5, NULL);
}

void Bluetooth::init(void *arg)
{
  Bluetooth *self = static_cast<Bluetooth *>(arg);

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
  }

  vTaskDelete(NULL);
}

void Bluetooth::deinit(void *arg)
{
  Bluetooth *self = static_cast<Bluetooth *>(arg);

  if (BLEDevice::getInitialized())
  {
    BLEDevice::stopAdvertising();
    BLEDevice::deinit();
  }

  vTaskDelete(NULL);
}

void Bluetooth::onReceive(CentralServices s, void *data)
{
  ESP_LOGI(TAG, "Bluetooth onReceive: %d", s);
  switch (s)
  {
  case CentralServices::STORAGE:
  {
    ServicePayload::RecievePayload<ServiceType::StorageEventType> *payload = static_cast<ServicePayload::RecievePayload<ServiceType::StorageEventType> *>(data);
    if (payload->type == ServiceType::StorageEventType::EVENT_STORAGE_STARTED)
    {
      /* start mesh service */
      // this->start();
    }
    delete payload;
    break;
  }
  case CentralServices::BUTTON:
  {
    ServicePayload::RecievePayload<ServiceType::ButtonType> *payload = static_cast<ServicePayload::RecievePayload<ServiceType::ButtonType> *>(data);
    if (payload->type == ServiceType::ButtonType::EVENT_BUTTON_CONFIG)
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