// SPDX-License-Identifier: GPL-2.0-or-later
// Ethernet-only network backend for the Waveshare ESP32-P4-Module-DEV-KIT.

#include "net.h"

#include <stdio.h>
#include <string.h>

#include "esp_eth.h"
#include "esp_eth_mac.h"
#include "esp_eth_phy.h"
#include "esp_eth_mac_esp.h"
#include "esp_eth_netif_glue.h"
#include "esp_check.h"
#include "esp_idf_version.h"
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(6, 0, 0)
#include "esp_eth_phy_ip101.h"
#endif
#include "esp_event.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_netif.h"
#include "esp_netif_ip_addr.h"
#include "nvs_flash.h"
#include "lwip/ip4_addr.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

static const char *TAG = "net-eth";
static EventGroupHandle_t s_events;
static esp_netif_t *s_netif;
static esp_eth_handle_t s_eth;
static esp_eth_netif_glue_handle_t s_glue;
static volatile bool s_connected;
static char s_ip[16] = "0.0.0.0";
static char s_host[24];

#define ETH_GOT_IP BIT0

static void net_event_handler(void *arg, esp_event_base_t base,
                              int32_t id, void *data)
{
    (void)arg;
    if (base == ETH_EVENT) {
        if (id == ETHERNET_EVENT_CONNECTED) {
            ESP_LOGI(TAG, "Ethernet link up");
        } else if (id == ETHERNET_EVENT_DISCONNECTED) {
            s_connected = false;
            snprintf(s_ip, sizeof(s_ip), "0.0.0.0");
            xEventGroupClearBits(s_events, ETH_GOT_IP);
            ESP_LOGW(TAG, "Ethernet link down");
        }
    } else if (base == IP_EVENT && id == IP_EVENT_ETH_GOT_IP) {
        const ip_event_got_ip_t *event = data;
        snprintf(s_ip, sizeof(s_ip), IPSTR, IP2STR(&event->ip_info.ip));
        s_connected = true;
        xEventGroupSetBits(s_events, ETH_GOT_IP);
        ESP_LOGI(TAG, "Ethernet got IPv4: %s", s_ip);
    }
}

esp_err_t net_init(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_RETURN_ON_ERROR(nvs_flash_erase(), TAG, "NVS erase failed");
        ESP_RETURN_ON_ERROR(nvs_flash_init(), TAG, "NVS init failed");
    } else if (err != ESP_OK) {
        return err;
    }

    err = esp_netif_init();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) return err;
    err = esp_event_loop_create_default();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) return err;

    s_events = xEventGroupCreate();
    if (!s_events) return ESP_ERR_NO_MEM;

    uint8_t mac_addr[6] = {0};
    ESP_RETURN_ON_ERROR(esp_read_mac(mac_addr, ESP_MAC_ETH), TAG, "Read Ethernet MAC failed");
    snprintf(s_host, sizeof(s_host), "rfnethm-%02X%02X", mac_addr[4], mac_addr[5]);

    esp_netif_config_t netif_cfg = ESP_NETIF_DEFAULT_ETH();
    s_netif = esp_netif_new(&netif_cfg);
    if (!s_netif) return ESP_ERR_NO_MEM;

    eth_mac_config_t mac_cfg = ETH_MAC_DEFAULT_CONFIG();
    eth_phy_config_t phy_cfg = ETH_PHY_DEFAULT_CONFIG();
    phy_cfg.phy_addr = 1;
    phy_cfg.reset_gpio_num = 51;

    eth_esp32_emac_config_t emac_cfg = ETH_ESP32_EMAC_DEFAULT_CONFIG();
    emac_cfg.smi_gpio.mdc_num = 31;
    emac_cfg.smi_gpio.mdio_num = 52;
    emac_cfg.interface = EMAC_DATA_INTERFACE_RMII;
    emac_cfg.clock_config.rmii.clock_mode = EMAC_CLK_EXT_IN;
    emac_cfg.clock_config.rmii.clock_gpio = 50;
    emac_cfg.emac_dataif_gpio.rmii.tx_en_num = 49;
    emac_cfg.emac_dataif_gpio.rmii.txd0_num = 34;
    emac_cfg.emac_dataif_gpio.rmii.txd1_num = 35;
    emac_cfg.emac_dataif_gpio.rmii.crs_dv_num = 28;
    emac_cfg.emac_dataif_gpio.rmii.rxd0_num = 29;
    emac_cfg.emac_dataif_gpio.rmii.rxd1_num = 30;

    esp_eth_mac_t *mac = esp_eth_mac_new_esp32(&emac_cfg, &mac_cfg);
    if (!mac) return ESP_FAIL;
    esp_eth_phy_t *phy = esp_eth_phy_new_ip101(&phy_cfg);
    if (!phy) {
        mac->del(mac);
        return ESP_FAIL;
    }
    esp_eth_config_t eth_cfg = ETH_DEFAULT_CONFIG(mac, phy);
    ESP_RETURN_ON_ERROR(esp_eth_driver_install(&eth_cfg, &s_eth), TAG, "Ethernet driver install failed");

    s_glue = esp_eth_new_netif_glue(s_eth);
    if (!s_glue) return ESP_ERR_NO_MEM;
    ESP_RETURN_ON_ERROR(esp_netif_attach(s_netif, s_glue), TAG, "Attach Ethernet netif failed");
    ESP_RETURN_ON_ERROR(esp_netif_set_hostname(s_netif, s_host), TAG, "Set hostname failed");
    ESP_RETURN_ON_ERROR(esp_event_handler_register(ETH_EVENT, ESP_EVENT_ANY_ID,
                                                   net_event_handler, NULL), TAG,
                        "Register Ethernet handler failed");
    ESP_RETURN_ON_ERROR(esp_event_handler_register(IP_EVENT, IP_EVENT_ETH_GOT_IP,
                                                   net_event_handler, NULL), TAG,
                        "Register IP handler failed");
    ESP_RETURN_ON_ERROR(esp_eth_start(s_eth), TAG, "Start Ethernet failed");
    ESP_LOGI(TAG, "Ethernet started; waiting for DHCP on RJ45");
    return ESP_OK;
}

bool net_is_connected(void) { return s_connected; }
const char *net_ip_str(void) { return s_ip; }
const char *net_ssid(void) { return "Ethernet"; }
esp_err_t net_persist_creds(const char *ssid, const char *psk)
{ (void)ssid; (void)psk; return ESP_ERR_NOT_SUPPORTED; }
void net_set_external_control(bool on) { (void)on; }
esp_err_t net_start_softap(void) { return ESP_ERR_NOT_SUPPORTED; }
bool net_is_ap_mode(void) { return false; }
const char *net_ap_ssid(void) { return ""; }
const char *net_hostname(void) { return s_host[0] ? s_host : "rfnethm"; }
esp_err_t net_scan(net_scan_entry_t *out, size_t cap, size_t *got)
{ (void)out; (void)cap; if (got) *got = 0; return ESP_ERR_NOT_SUPPORTED; }
esp_err_t net_clear_creds(void) { return ESP_ERR_NOT_SUPPORTED; }
bool net_has_creds(void) { return false; }
