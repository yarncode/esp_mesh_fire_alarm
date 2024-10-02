#include "Mesh.hpp"
#include "ApiCaller.hpp"
#include "SNTP.hpp"
#include "Cache.h"
#include "Helper.hpp"
#include "Storage.hpp"

#include <iostream>
#include <string>
#include <vector>
#include <map>

using namespace ServicePayload;
using namespace ServiceType;

Mesh *Mesh::_instance = nullptr;
const uint8_t Mesh::meshId[6] = {0x77, 0x77, 0x77, 0x77, 0x77, 0x76};
const std::string Mesh::meshPassword = "fire_alarm";
static QueueHandle_t _queueReciveMsg = nullptr;
static TaskHandle_t _taskRecieveMsg = nullptr;
static TaskHandle_t _taskTxDemo = nullptr;

/* router config */
const std::string Mesh::ssid = "duy123";
const std::string Mesh::password = "44448888";

const std::string Mesh::ssid_2 = "Demo box";
const std::string Mesh::password_2 = "demobox123";

static const char *TAG = "Mesh";

typedef struct
{
  mesh_addr_t *from;
  mesh_data_t *data;
} mesh_custom_data_t;

void free_mesh_custom_data(mesh_custom_data_t *data)
{
  if (data == NULL)
  {
    return;
  }
  free(data->data);
  free(data->from);
  free(data);
}

void Mesh::transmitDemo(void *arg)
{
  Mesh *self = static_cast<Mesh *>(arg);
  const char *payload = "Hello World";
  while (true)
  {
    /* send message from node -> root */
    if (esp_mesh_is_root() == false)
    {
      ESP_LOGI(TAG, "Sending message to ROOT...");
      self->sendMessage(NULL, (uint8_t *)payload, strlen(payload));
    }
    vTaskDelay((5 * 1000) / portTICK_PERIOD_MS);
  }
  vTaskDelete(NULL);
}

void Mesh::ip_event_handler(void *arg, esp_event_base_t event_base,
                            int32_t event_id, void *event_data)
{
  Mesh *self = static_cast<Mesh *>(arg); // get instance

  /* get api instance */
  ApiCaller *api = static_cast<ApiCaller *>(self->getObserver(CentralServices::API_CALLER));
  Sntp *sntp = static_cast<Sntp *>(self->getObserver(CentralServices::SNTP));

  ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
  ESP_LOGI(TAG, "<IP_EVENT_STA_GOT_IP>IP:" IPSTR, IP2STR(&event->ip_info.ip));
  self->currentIp.addr = event->ip_info.ip.addr;

  esp_netif_t *netif = event->esp_netif;
  esp_netif_dns_info_t dns;
  ESP_ERROR_CHECK(esp_netif_get_dns_info(netif, ESP_NETIF_DNS_MAIN, &dns));
  mesh_netif_start_root_ap(esp_mesh_is_root(), dns.ip.u_addr.ip4.addr);

  cacheManager.ip = ip4ToString(event->ip_info.ip);
  cacheManager.netmask = ip4ToString(event->ip_info.netmask);
  cacheManager.gateway = ip4ToString(event->ip_info.gw);

  /* save wifi config */
  Storage storage;
  storage.saveWifiConfig();

  /* ignore if state api is creating device */
  if (api->isCallingCreateDevice() == false)
  {
    /* start MQTT service */
    self->notify(self->_service, CentralServices::MQTT, new RecievePayload_2<MqttEventType, nullptr_t>(MqttEventType::EVENT_MQTT_START, nullptr));
  }

  if (cacheManager.activeToken.length() > 0)
  {
    /* add device to server */
    self->notify(self->_service, CentralServices::API_CALLER, new RecievePayload_2<ApiCallerType, nullptr_t>(ApiCallerType::EVENT_API_CALLER_ADD_DEVICE, nullptr));
  }

  /* start SNTP service */
  if (sntp->sntpState() == false)
  {
    /* start service time if not running */
    sntp->start();
  }
  else
  {
    /* just restart service */
    sntp->restart();
  }
}

void Mesh::mesh_event_handler(void *arg, esp_event_base_t event_base,
                              int32_t event_id, void *event_data)
{
  Mesh *self = static_cast<Mesh *>(arg); // get instance

  switch (event_id)
  {
  case MESH_EVENT_STARTED:
  {
    esp_mesh_get_id(&self->meshAddress);
    ESP_LOGI(TAG, "<MESH_EVENT_MESH_STARTED>ID:" MACSTR "", MAC2STR(self->meshAddress.addr));
    self->_isNativeStart = true;
    self->layer = esp_mesh_get_layer();
    break;
  }
  case MESH_EVENT_STOPPED:
  {
    ESP_LOGI(TAG, "<MESH_EVENT_STOPPED>");
    self->layer = esp_mesh_get_layer();
    self->_isNativeStart = false;
    break;
  }
  case MESH_EVENT_CHILD_CONNECTED:
  {
    mesh_event_child_connected_t *child_connected = (mesh_event_child_connected_t *)event_data;
    ESP_LOGI(TAG, "<MESH_EVENT_CHILD_CONNECTED>aid:%d, " MACSTR "",
             child_connected->aid,
             MAC2STR(child_connected->mac));
    break;
  }
  case MESH_EVENT_CHILD_DISCONNECTED:
  {
    mesh_event_child_disconnected_t *child_disconnected = (mesh_event_child_disconnected_t *)event_data;
    ESP_LOGI(TAG, "<MESH_EVENT_CHILD_DISCONNECTED>aid:%d, " MACSTR "",
             child_disconnected->aid,
             MAC2STR(child_disconnected->mac));
    break;
  }
  case MESH_EVENT_ROUTING_TABLE_ADD:
  {
    mesh_event_routing_table_change_t *routing_table = (mesh_event_routing_table_change_t *)event_data;
    ESP_LOGW(TAG, "<MESH_EVENT_ROUTING_TABLE_ADD>add %d, new:%d",
             routing_table->rt_size_change,
             routing_table->rt_size_new);
    break;
  }
  case MESH_EVENT_ROUTING_TABLE_REMOVE:
  {
    mesh_event_routing_table_change_t *routing_table = (mesh_event_routing_table_change_t *)event_data;
    ESP_LOGW(TAG, "<MESH_EVENT_ROUTING_TABLE_REMOVE>remove %d, new:%d",
             routing_table->rt_size_change,
             routing_table->rt_size_new);
    break;
  }
  case MESH_EVENT_NO_PARENT_FOUND:
  {
    mesh_event_no_parent_found_t *no_parent = (mesh_event_no_parent_found_t *)event_data;
    ESP_LOGI(TAG, "<MESH_EVENT_NO_PARENT_FOUND>scan times:%d",
             no_parent->scan_times);
    break;
  }
  case MESH_EVENT_PARENT_CONNECTED:
  {
    mesh_event_connected_t *connected = (mesh_event_connected_t *)event_data;
    esp_mesh_get_id(&self->meshAddress);
    self->layer = connected->self_layer;
    memcpy(&self->parentAddress.addr, connected->connected.bssid, 6);
    ESP_LOGI(TAG,
             "<MESH_EVENT_PARENT_CONNECTED>layer:%d-->%d, parent:" MACSTR "%s, ID:" MACSTR "",
             self->lastLayer, self->layer, MAC2STR(self->parentAddress.addr),
             esp_mesh_is_root() ? "<ROOT>" : (self->layer == 2) ? "<layer2>"
                                                                : "",
             MAC2STR(self->meshAddress.addr));
    self->lastLayer = self->layer;
    self->_isParentConnected = true;
    mesh_netifs_start(esp_mesh_is_root());

    /* start task sending demo */
    // if (esp_mesh_is_root() == false)
    // {
    //   xTaskCreate(Mesh::transmitDemo, "transmitDemo", 4 * 1024, self, 8, &_taskTxDemo);
    // }
    break;
  }
  case MESH_EVENT_PARENT_DISCONNECTED:
  {
    mesh_event_disconnected_t *disconnected = (mesh_event_disconnected_t *)event_data;
    ESP_LOGI(TAG,
             "<MESH_EVENT_PARENT_DISCONNECTED>reason:%d",
             disconnected->reason);
    self->_isParentConnected = false;
    self->layer = esp_mesh_get_layer();
    mesh_netifs_stop(false);

    /* clear task sending demo */
    // if (_taskTxDemo != nullptr)
    // {
    //   vTaskDelete(_taskTxDemo);
    //   _taskTxDemo = nullptr;
    // }
    break;
  }
  case MESH_EVENT_LAYER_CHANGE:
  {
    mesh_event_layer_change_t *layer_change = (mesh_event_layer_change_t *)event_data;
    self->layer = layer_change->new_layer;
    ESP_LOGI(TAG, "<MESH_EVENT_LAYER_CHANGE>layer:%d-->%d%s",
             self->lastLayer, self->layer,
             esp_mesh_is_root() ? "<ROOT>" : (self->layer == 2) ? "<layer2>"
                                                                : "");
    self->lastLayer = self->layer;
    self->layer = self->layer;
    break;
  }
  case MESH_EVENT_ROOT_ADDRESS:
  {
    mesh_event_root_address_t *root_addr = (mesh_event_root_address_t *)event_data;
    ESP_LOGI(TAG, "<MESH_EVENT_ROOT_ADDRESS>root address:" MACSTR "",
             MAC2STR(root_addr->addr));
    break;
  }
  case MESH_EVENT_VOTE_STARTED:
  {
    mesh_event_vote_started_t *vote_started = (mesh_event_vote_started_t *)event_data;
    ESP_LOGI(TAG,
             "<MESH_EVENT_VOTE_STARTED>attempts:%d, reason:%d, rc_addr:" MACSTR "",
             vote_started->attempts,
             vote_started->reason,
             MAC2STR(vote_started->rc_addr.addr));
    break;
  }
  case MESH_EVENT_VOTE_STOPPED:
  {
    ESP_LOGI(TAG, "<MESH_EVENT_VOTE_STOPPED>");
    break;
  }
  case MESH_EVENT_ROOT_SWITCH_REQ:
  {
    mesh_event_root_switch_req_t *switch_req = (mesh_event_root_switch_req_t *)event_data;
    ESP_LOGI(TAG,
             "<MESH_EVENT_ROOT_SWITCH_REQ>reason:%d, rc_addr:" MACSTR "",
             switch_req->reason,
             MAC2STR(switch_req->rc_addr.addr));
    break;
  }
  case MESH_EVENT_ROOT_SWITCH_ACK:
  {
    /* new root */
    self->layer = esp_mesh_get_layer();
    esp_mesh_get_parent_bssid(&self->parentAddress);
    ESP_LOGI(TAG, "<MESH_EVENT_ROOT_SWITCH_ACK>layer:%d, parent:" MACSTR "", self->layer, MAC2STR(self->parentAddress.addr));
    break;
  }
  case MESH_EVENT_TODS_STATE:
  {
    mesh_event_toDS_state_t *toDs_state = (mesh_event_toDS_state_t *)event_data;
    ESP_LOGI(TAG, "<MESH_EVENT_TODS_REACHABLE>state:%d", *toDs_state);
    break;
  }
  case MESH_EVENT_ROOT_FIXED:
  {
    mesh_event_root_fixed_t *root_fixed = (mesh_event_root_fixed_t *)event_data;
    ESP_LOGI(TAG, "<MESH_EVENT_ROOT_FIXED>%s",
             root_fixed->is_fixed ? "fixed" : "not fixed");
    break;
  }
  case MESH_EVENT_ROOT_ASKED_YIELD:
  {
    mesh_event_root_conflict_t *root_conflict = (mesh_event_root_conflict_t *)event_data;
    ESP_LOGI(TAG,
             "<MESH_EVENT_ROOT_ASKED_YIELD>" MACSTR ", rssi:%d, capacity:%d",
             MAC2STR(root_conflict->addr),
             root_conflict->rssi,
             root_conflict->capacity);
    break;
  }
  case MESH_EVENT_CHANNEL_SWITCH:
  {
    mesh_event_channel_switch_t *channel_switch = (mesh_event_channel_switch_t *)event_data;
    ESP_LOGI(TAG, "<MESH_EVENT_CHANNEL_SWITCH>new channel:%d", channel_switch->channel);
    break;
  }
  case MESH_EVENT_SCAN_DONE:
  {
    mesh_event_scan_done_t *scan_done = (mesh_event_scan_done_t *)event_data;
    ESP_LOGI(TAG, "<MESH_EVENT_SCAN_DONE>number:%d",
             scan_done->number);
    break;
  }
  case MESH_EVENT_NETWORK_STATE:
  {
    mesh_event_network_state_t *network_state = (mesh_event_network_state_t *)event_data;
    ESP_LOGI(TAG, "<MESH_EVENT_NETWORK_STATE>is_rootless:%d",
             network_state->is_rootless);
    break;
  }
  case MESH_EVENT_STOP_RECONNECTION:
  {
    ESP_LOGI(TAG, "<MESH_EVENT_STOP_RECONNECTION>");
    break;
  }
  case MESH_EVENT_FIND_NETWORK:
  {
    mesh_event_find_network_t *find_network = (mesh_event_find_network_t *)event_data;
    ESP_LOGI(TAG, "<MESH_EVENT_FIND_NETWORK>new channel:%d, router BSSID:" MACSTR "",
             find_network->channel, MAC2STR(find_network->router_bssid));
    break;
  }
  case MESH_EVENT_ROUTER_SWITCH:
  {
    mesh_event_router_switch_t *router_switch = (mesh_event_router_switch_t *)event_data;
    ESP_LOGI(TAG, "<MESH_EVENT_ROUTER_SWITCH>new router:%s, channel:%d, " MACSTR "",
             router_switch->ssid, router_switch->channel, MAC2STR(router_switch->bssid));
    break;
  }
  default:
    ESP_LOGI(TAG, "unknown id:%" PRId32 "", event_id);
    break;
  }
}

/**
 * @brief Callback function for mesh data receive
 *
 * This function is called when the mesh network receives a data message.
 * It will copy the received data and send it to the `_queueReciveMsg` queue.
 *
 * @param from pointer to a `mesh_addr_t` structure containing the source MAC address
 * @param data pointer to a `mesh_data_t` structure containing the received data
 *
 * This function is non-blocking and does not return any status.
 */
void Mesh::callbackData(mesh_addr_t *from, mesh_data_t *data)
{
  /* step 1: malloc mesh_custom_data_t pointer */
  mesh_custom_data_t *custom_data = (mesh_custom_data_t *)malloc(sizeof(mesh_custom_data_t));
  if (custom_data == NULL)
  {
    ESP_LOGE(TAG, "Malloc mesh_custom_data_t failed.");
    return;
  }
  custom_data->from = (mesh_addr_t *)malloc(sizeof(mesh_addr_t));
  custom_data->data = (mesh_data_t *)malloc(sizeof(mesh_data_t) + (data->size * sizeof(uint8_t)));

  /* step 2: copy data */
  memcpy(custom_data->from, from, sizeof(mesh_addr_t));
  memcpy(custom_data->data, data, sizeof(mesh_data_t) + (data->size * sizeof(uint8_t)));

  /* step 3: send to queue */
  if (_queueReciveMsg != NULL)
  {
    if (xQueueSend(_queueReciveMsg, &custom_data, 0) != pdPASS)
    {
      ESP_LOGE(TAG, "Queue {_queueReciveMsg} send failed");
      free_mesh_custom_data(custom_data);

      /* clear queue */
      xQueueReset(_queueReciveMsg);
    }
  }
}

/**
 * @brief Send a message to another node in the mesh network
 *
 * @param addr MAC address of the node to send to
 * @param data pointer to the data to send
 * @param len length of the data to send
 *
 * Uses the MESH_DATA_P2P protocol to send the data directly to the specified
 * node. This function is non-blocking and does not return any status.
 */
void Mesh::sendMessage(uint8_t *addr, uint8_t *data, uint16_t len)
{
  mesh_addr_t to;
  mesh_data_t payload;
  payload.data = data;
  payload.size = len;
  payload.proto = MESH_PROTO_JSON; // sending from root AP -> Node's STA
  payload.tos = MESH_TOS_P2P;
  if (addr != NULL)
  {
    memcpy(to.addr, addr, 6);
  }
  esp_err_t err = esp_mesh_send(addr ? &to : NULL, &payload, MESH_DATA_P2P, NULL, 0);
  if (ESP_OK != err)
  {
    ESP_LOGE(TAG, "Send with err code %d %s", err, esp_err_to_name(err));
  }
}

void Mesh::recieveMsg(void *arg)
{
  Mesh *self = static_cast<Mesh *>(arg);
  mesh_custom_data_t *data;

  while (true)
  {
    if (_queueReciveMsg == NULL)
    {
      ESP_LOGW(TAG, "Queue recieve msg is null.");
      vTaskDelay(1000 / portTICK_PERIOD_MS);
      continue;
    }
    if (xQueueReceive(_queueReciveMsg, &data, portMAX_DELAY) == pdPASS)
    {
      try
      {
        std::string payload((char *)data->data->data, (int)(data->data->size));
        ESP_LOGI(TAG, "Mesh Data from: [" MACSTR "] => %s", MAC2STR(data->from->addr), payload.c_str());
        /* handle message from here */
      }
      catch (const std::exception &e)
      {
        std::cerr << e.what() << '\n';
      }

      free_mesh_custom_data(data); // release memory
    }
    vTaskDelay(50 / portTICK_PERIOD_MS);
  }
}

void Mesh::start(void)
{
  if (this->_isStarted)
  {
    ESP_LOGW(TAG, "Mesh already started");
    return;
  }
  ESP_LOGI(TAG, "Mesh start");
  xTaskCreate(&Mesh::init, "Mesh::init", 4 * 1024, this, 5, NULL);
}

void Mesh::stop(void)
{
  if (!this->_isStarted)
  {
    ESP_LOGW(TAG, "Mesh already stopped");
    return;
  }
  ESP_LOGI(TAG, "Mesh stop");
  xTaskCreate(&Mesh::deinit, "Mesh::deinit", 4 * 1024, this, 5, NULL);
}

void Mesh::deinit(void *arg)
{
  Mesh *self = static_cast<Mesh *>(arg);

  self->stopWifiAndMesh();

  self->_isStarted = false; // stop service
  vTaskDelete(NULL);
}

bool Mesh::isStarted(void)
{
  return _instance->_isNativeStart;
}

void Mesh::initWiFiNative(void)
{
  if (this->_isStartBaseWiFi)
  {
    return;
  }
  ESP_LOGI(TAG, "Start wifi native...");

  wifi_init_config_t config = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&config));
  ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &Mesh::ip_event_handler, this));
  ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_FLASH));
  ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));

  this->_isStartBaseWiFi = true;
}

void Mesh::deinitWiFiNative(void)
{
  if (this->_isStartBaseWiFi == false)
  {
    return;
  }
  ESP_LOGI(TAG, "Deinit wifi native...");

  ESP_ERROR_CHECK(esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, &Mesh::ip_event_handler));
  ESP_ERROR_CHECK(esp_wifi_stop());
  ESP_ERROR_CHECK(esp_wifi_deinit());

  this->_isStartBaseWiFi = false;
}

void Mesh::initMeshNative(void)
{
  if (this->_isStartBaseMesh)
  {
    return;
  }

  ESP_LOGI(TAG, "Start mesh native...");

  ESP_ERROR_CHECK(esp_mesh_init());
  ESP_ERROR_CHECK(esp_event_handler_register(MESH_EVENT, ESP_EVENT_ANY_ID, &Mesh::mesh_event_handler, this));
  ESP_ERROR_CHECK(esp_mesh_set_max_layer(CONFIG_MESH_MAX_LAYER));
  ESP_ERROR_CHECK(esp_mesh_set_vote_percentage(1));
  ESP_ERROR_CHECK(esp_mesh_set_ap_assoc_expire(10));
  mesh_cfg_t cfg = MESH_INIT_CONFIG_DEFAULT();

  cfg.allow_channel_switch = true;
  cfg.crypto_funcs = NULL;

  /* router */
  memcpy((uint8_t *)&cfg.mesh_id, this->meshId, 6);
  cfg.router.ssid_len = cacheManager.ssid.length();
  cfg.router.allow_router_switch = true;
  memcpy((uint8_t *)&cfg.router.ssid, cacheManager.ssid.c_str(), cfg.router.ssid_len);
  memcpy((uint8_t *)&cfg.router.password, cacheManager.password.c_str(), cacheManager.password.length());

  /* mesh softAP */
  ESP_ERROR_CHECK(esp_mesh_set_ap_authmode(WIFI_AUTH_WPA2_PSK));
  cfg.mesh_ap.max_connection = 6;
  cfg.mesh_ap.nonmesh_max_connection = 0;
  memcpy((uint8_t *)&cfg.mesh_ap.password, this->meshPassword.c_str(), this->meshPassword.length());
  ESP_ERROR_CHECK(esp_mesh_set_config(&cfg));

  this->_isStartBaseMesh = true;
}

void Mesh::deinitMeshNative(void)
{
  if (this->_isStartBaseMesh == false)
  {
    return;
  }
  ESP_LOGI(TAG, "Deinit mesh native...");

  ESP_ERROR_CHECK(esp_event_handler_unregister(MESH_EVENT, ESP_EVENT_ANY_ID, &Mesh::mesh_event_handler));
  ESP_ERROR_CHECK(esp_mesh_stop());
  ESP_ERROR_CHECK(esp_mesh_deinit());
  ESP_ERROR_CHECK(mesh_netifs_stop(true));
  ESP_ERROR_CHECK(mesh_netifs_destroy());

  this->_isStartBaseMesh = false;
}

void Mesh::startWiFi(void)
{
  this->initWiFiNative();
  ESP_ERROR_CHECK(esp_wifi_start());
}

void Mesh::startMesh(void)
{
  this->initMeshNative();
  // /* mesh start */
  ESP_ERROR_CHECK(esp_mesh_start());
}

void Mesh::startBase(void)
{
  if (this->_isStartBase)
  {
    return;
  }

  ESP_LOGI(TAG, "Start base...");
  if (this->_isStartTCP == false)
  {
    /*  tcpip initialization */
    ESP_ERROR_CHECK(esp_netif_init());
    this->_isStartTCP = true;
  }

  if (this->_isStartLoopDefault == false)
  {
    /*  event initialization */
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    this->_isStartLoopDefault = true;
  }

  if (_queueReciveMsg == nullptr)
  {
    _queueReciveMsg = xQueueCreate(10, sizeof(mesh_custom_data_t *));
  }

  if (_taskRecieveMsg == nullptr)
  {
    xTaskCreate(&this->recieveMsg, "recieveMsg", 4 * 1024, this, 5, &_taskRecieveMsg);
  }

  // if (this->_isStartNetIF == false)
  // {
  /*  crete network interfaces for mesh (only station instance saved for further manipulation, soft AP instance ignored */
  mesh_netifs_init(this->callbackData);
  //   this->_isStartNetIF = true;
  // }

  this->_isStartBase = true;
}

void Mesh::deinitBase(void)
{
  if (this->_isStartBase == false)
  {
    return;
  }

  if (_queueReciveMsg != nullptr)
  {
    xQueueReset(_queueReciveMsg);
  }

  if (_taskRecieveMsg != nullptr)
  {
    vTaskDelete(_taskRecieveMsg);
    _taskRecieveMsg = nullptr;
  }

  ESP_LOGI(TAG, "Deinit base...");
  this->_isStartBase = false;
}

void Mesh::init(void *arg)
{
  Mesh *self = static_cast<Mesh *>(arg);

  /* start setup default startup */
  self->startBase();

  if (cacheManager.ssid.length() == 0)
  {
    ESP_LOGW(TAG, "WiFi config not found.");
    vTaskDelete(NULL);
    return;
  }

  /*  wifi initialization */
  self->startWiFi();

  /*  mesh initialization */
  self->startMesh();

  self->_isStarted = true; // start service
  vTaskDelete(NULL);
}

void Mesh::stopWifiAndMesh(void)
{
  this->deinitWiFiNative();
  this->deinitMeshNative();
  this->deinitBase();
  this->_isStarted = false;
}

void Mesh::handleFactory(void *arg)
{
  ESP_LOGW(TAG, "Mesh handle factory...");
  vTaskDelete(NULL);
}

void Mesh::onReceive(CentralServices s, void *data)
{
  std::cout << "Mesh onReceive from " << s << std::endl;
  switch (s)
  {
  case CentralServices::REFACTOR:
  {
    xTaskCreate(this->handleFactory, "handleFactory", 4 * 1024, this, 10, NULL);
    break;
  }

  case CentralServices::BLUETOOTH:
  {
    RecievePayload_2<MeshEventType, nullptr_t> *payload = static_cast<RecievePayload_2<MeshEventType, nullptr_t> *>(data);

    if (payload->type == EVENT_MESH_START)
    {
      ESP_LOGI(TAG, "BLUETOOTH request Mesh started.");
      if (this->_isStarted == false)
      {
        /* call func update router config */

        /* and then start service mesh again */
        this->start();
      }
    }
    delete payload;
    break;
  }

  case CentralServices::STORAGE:
  {
    RecievePayload_2<MeshEventType, nullptr_t> *payload = static_cast<RecievePayload_2<MeshEventType, nullptr_t> *>(data);

    if (payload->type == EVENT_MESH_START)
    {
      ESP_LOGI(TAG, "STORAGE request Mesh started.");
      if (this->_isStarted == false)
      {
        /* call func update router config */

        /* and then start service mesh again */
        this->start();
      }
    }
    delete payload;
    break;
  }

  default:
    break;
  }
}