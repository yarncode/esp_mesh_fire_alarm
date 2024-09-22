set(
  SOURCE 
  "Observer/Observer.cpp"
  "Utils/Helper.cpp"
  "Template/Template.cpp"
  "Storage/Storage.cpp"
  "Storage/Cache.cpp"
  "Mesh/Mesh.cpp"
  "Mesh/mesh_netif.c"
  "Log/Log.cpp"
  "Mqtt/Mqtt.cpp"
  "Bluetooth/Bluetooth.cpp"
  "Refactor/Refactor.cpp"
  "Button/Button.cpp"
  "Led/Led.cpp"
)

set(
  HEADERS 
  "Config"
  "Utils/include"
  "Observer/include"
  "Template/include"
  "Storage/include"
  "Mesh/include"
  "Log/include"
  "Mqtt/include"
  "Bluetooth/include"
  "Refactor/include"
  "Button/include"
  "Led/include"
)

idf_component_register(SRCS "main.cpp" ${SOURCE} ${NATIVE_LIBS} INCLUDE_DIRS "." ${HEADERS})