#ifndef __BLE_HOST_H__
#define __BLE_HOST_H__

#include "esp_err.h"

struct ble_hs_cfg;
struct ble_gatt_register_ctxt;

void gatt_svr_register_cb(struct ble_gatt_register_ctxt *ctxt, void *arg);
int gatt_svr_init(void);

void ble_init();


#endif /* __BLE_HOST_H */