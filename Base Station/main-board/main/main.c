#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_event.h"

/* Bluetooth LE Libraries, make sure BT is enabled*/
// #include "esp_bt.h"
// #include "esp_gap_ble_api.h"
// #include "esp_gatts_api.h"
// #include "esp_bt_defs.h"
// #include "esp_bt_main.h"
// #include "esp_bt_device.h"
// #include "esp_gatt_common_api.h"
// #include "sdkconfig.h"
#include "ble_host.h"

/* WiFi Libraries*/
#include "esp_mac.h"
#include "esp_wifi.h"
#include "esp_netif_net_stack.h"
#include "esp_netif.h"
#include "nvs_flash.h"
#include "lwip/inet.h"
#include "lwip/netdb.h"
#include "lwip/sockets.h"
#if IP_NAPT
#include "lwip/lwip_napt.h"
#endif
#include "lwip/err.h"
#include "lwip/sys.h"

void app_main(void)
{
    esp_err_t esp_status;

    esp_status = nvs_flash_init();
    if (esp_status == ESP_ERR_NVS_NO_FREE_PAGES || esp_status == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        esp_status = nvs_flash_init();
    }
    ESP_ERROR_CHECK(esp_status);

    esp_status = ble_init();
}