#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>

#include "Storage.hpp"
#include "Mesh.hpp"
#include "Log.hpp"
#include "Mqtt.hpp"
#include "Bluetooth.hpp"
#include "Refactor.hpp"
#include "Button.hpp"

extern "C" void app_main(void)
{
    Storage storage;
    Mesh mesh;
    Log logger;
    Mqtt mqtt;
    Bluetooth ble;
    Refactor factory;
    Button button;

    /* init storage */
    storage.registerTwoWayObserver(&mesh, CentralServices::MESH);
    storage.registerTwoWayObserver(&logger, CentralServices::LOG);
    storage.registerTwoWayObserver(&ble, CentralServices::BLUETOOTH);
    mesh.registerTwoWayObserver(&mqtt, CentralServices::MQTT);
    button.registerTwoWayObserver(&ble, CentralServices::BLUETOOTH);
    logger.registerObserver(&mesh, CentralServices::MESH);
    ble.registerTwoWayObserver(&mesh, CentralServices::MESH);
    ble.registerTwoWayObserver(&mqtt, CentralServices::MQTT);

    mqtt.registerObserver(&factory, CentralServices::REFACTOR);

    factory.registerObserver(&mesh, CentralServices::MESH);
    factory.registerObserver(&storage, CentralServices::STORAGE);

    storage.start(); // run first storage
    button.start();  // run button

    /* keep alive main process */
    while (true)
    {
        vTaskDelay(portMAX_DELAY);
    }
}