#include "ble_host.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_log.h"
// #include "nvs_flash.h"

#include "esp_bt.h"
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_bt_defs.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"
#include "esp_gatt_common_api.h"
#include "sdkconfig.h"

#define GATTS_TAG "GATT_Test"

esp_err_t ble_init(void) {
    esp_err_t esp_status;
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    esp_status = esp_bt_controller_init(&bt_cfg);
    if (esp_status) {
        ESP_LOGE(GATTS_TAG, "%s initialize controller failed: %s", __func__, esp_err_to_name(esp_status));
        return esp_status;
    }
    esp_status = esp_bt_controller_enable(ESP_BT_MODE_BLE);
    if (esp_status) {
        ESP_LOGE(GATTS_TAG, "%s enable controller failed: %s", __func__, esp_err_to_name(esp_status));
        return esp_status;
    }
    esp_status = esp_bluedroid_init();
    if (esp_status) {
        ESP_LOGE(GATTS_TAG, "%s initialize bluetooth failed: %s", __func__, esp_err_to_name(esp_status));
        return esp_status;
    }
    esp_status = esp_bluedroid_enable();
    if (esp_status) {
        ESP_LOGE(GATTS_TAG, "%s enable bluetooth failed: %s", __func__, esp_err_to_name(esp_status));
        return esp_status;
    }
    return esp_status;
}