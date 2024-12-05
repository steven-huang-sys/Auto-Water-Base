#ifndef __SHARED_VARIABLES_H__
#define __SHARED_VARIABLES_H__

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "socket.h"

#define DEVICE_ARRAY_SIZE      10
#define IP_STR_SIZE            INET_ADDRSTRLEN
#define MAC_STR_SIZE           18
#define STA_STR_SIZE           30
#define COMMAND_BUFFER         50

struct devices {
    char ipaddr_str[IP_STR_SIZE];
    char mac_str[MAC_STR_SIZE];
    char command[COMMAND_BUFFER];
    float temperature;
    int moisture;
    int uv;
};

struct sta_fields {
    char ssid[STA_STR_SIZE];
    char password[STA_STR_SIZE]; // for anyone reading this in the future, this is NOT how you should do WiFi provisioning
    char identifier[257];
};

// extern char ble_payload[1024];
// extern char * ble_payload_ptr;
// extern char ble_in[1024];
// extern int ble_numbytes;
extern struct sta_fields sta_info;
extern struct devices connected_devices[DEVICE_ARRAY_SIZE];

extern int state;
extern SemaphoreHandle_t bufferMutex;
extern SemaphoreHandle_t bufferMutex1;


#endif /* __SHARED_VARIABLES_H__ */