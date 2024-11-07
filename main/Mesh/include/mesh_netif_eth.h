/* Mesh IP Internal Networking Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#pragma once

#include "esp_mesh.h"

/*******************************************************
 *                Macros
 *******************************************************/
#define MAC_ADDR_LEN (6u)
#define MAC_ADDR_EQUAL(a, b) (0 == memcmp(a, b, MAC_ADDR_LEN))

#ifdef __cplusplus
extern "C"
{
#endif

    /*******************************************************
     *                Type Definitions
     *******************************************************/
    typedef void(mesh_raw_recv_cb_t)(mesh_addr_t *from, mesh_data_t *data);

    /*******************************************************
     *                Function Declarations
     *******************************************************/

    /**
     * @brief Initializes netifs in a default way before knowing if we are going to be a root
     *
     * @param cb callback receive function for mesh raw packets
     *
     * @return ESP_OK on success
     */
    esp_netif_t *mesh_netifs_eth_init(mesh_raw_recv_cb_t *cb);

    /**
     * @brief Destroy the netifs and related structures
     *
     * @return ESP_OK on success
     */
    esp_err_t mesh_netifs_eth_destroy(void);

    /**
     * @brief Start the mesh netifs based on the configuration (root/node)
     *
     * @return ESP_OK on success
     */
    esp_err_t mesh_netifs_eth_start(bool is_root);

    /**
     * @brief Stop the netifs and reset to the default mode
     *
     * @return ESP_OK on success
     */
    esp_err_t mesh_netifs_eth_stop(bool disable_reverse);

    /**
     * @brief Start the netif for root AP
     *
     * Note: The AP netif needs to be started separately after root received
     * an IP address from the router so the DNS address could be used for dhcps
     *
     * @param is_root must be true, ignored otherwise
     * @param dns_addr DNS address to use in DHCP server running on roots AP
     *
     * @return ESP_OK on success
     */
    esp_err_t mesh_netif_eth_start_root_ap(bool is_root, uint32_t dns_addr);

    /**
     * @brief Returns MAC address of the station interface
     *
     * Used mainly for checking node addresses of the peers in routing table
     * to avoid sending data to oneself
     *
     * @return Pointer to MAC address
     */
    uint8_t *mesh_netif_eth_get_station_mac(void);

#ifdef __cplusplus
}
#endif