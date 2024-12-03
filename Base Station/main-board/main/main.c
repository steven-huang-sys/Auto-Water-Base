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

/* Bluetooth LE Module, make sure BT is enabled */
#include "ble_host.h"

/* WiFi Module */
#include "wifi_base.h"

/* Display Module */
#include "spi_display.h"

/* Buttons or Control Pad Module*/
#include "hub_controller.h"

/* Internet Clients */
#include "udp_client.h"
#include "http_client.h"

void app_main(void)
{
    
    control_init();

    esp_err_t esp_status;

    esp_status = nvs_flash_init();
    if (esp_status == ESP_ERR_NVS_NO_FREE_PAGES || esp_status == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        esp_status = nvs_flash_init();
    }
    ESP_ERROR_CHECK(esp_status);

    display_init();
    ble_init();
    wifi_init();
    // http_client_init();
    udp_client_init();

}