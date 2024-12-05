/* HTTP GET Example using plain POSIX sockets

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
// #include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"
#include "sdkconfig.h"

#include "shared_variables.h"
#include "cjson.h"

/* Constants that aren't configurable in menuconfig */
#define WEB_SERVER "192.168.1.56"
#define WEB_PORT 5000
// #define WEB_PORT_STR "5000"
#define WEB_PATH "/data"
#define BUFFER_SIZE 1024

static const char *TAG = "HTTP Client";

// static const char *REQUEST = "PUT " WEB_PATH " HTTP/1.0\r\n"
//     "Host: "WEB_SERVER":"WEB_PORT_STR"\r\n"
//     // "User-Agent: esp-idf/1.0 esp32\r\n"
//     "\r\n";

void http_json_read(char * http_buffer) {
    cJSON *json = cJSON_ParseWithLength(http_buffer, BUFFER_SIZE);
    if (json == NULL) {
        ESP_LOGE(TAG, "Error parsing JSON\n");
        return;
    }

    // Extract the "message" string
    cJSON *message = cJSON_GetObjectItemCaseSensitive(json, "message");
    if (cJSON_IsString(message) && (message->valuestring != NULL)) {
        ESP_LOGI(TAG, "message: %s\n", message->valuestring);
    } else {
        ESP_LOGE(TAG, "message is not a valid string\n");
    }

    // Extract the "commands" array
    cJSON *commands = cJSON_GetObjectItemCaseSensitive(json, "commands");
    if (!cJSON_IsArray(commands)) {
        ESP_LOGW(TAG, "Commands is not an array\n");
        cJSON_Delete(json);
        return;
    }

    // Extract the first item in the "commands" array
    cJSON *command = cJSON_GetArrayItem(commands, 0);
    if (command == NULL) {
        ESP_LOGW(TAG, "Command item not found\n");
        cJSON_Delete(json);
        return;
    }

    // Extract "macAddress" and "type" from the command
    cJSON *macAddress = cJSON_GetObjectItemCaseSensitive(command, "macAddress");
    cJSON *type = cJSON_GetObjectItemCaseSensitive(command, "type");

    if (cJSON_IsString(macAddress) && (macAddress->valuestring != NULL)) {
        ESP_LOGI(TAG, "macAddress: %s\n", macAddress->valuestring);
    } else {
        ESP_LOGE(TAG, "macAddress is not a valid string\n");
    }

    if (cJSON_IsString(type) && (type->valuestring != NULL)) {
        ESP_LOGI(TAG, "type: %s\n", type->valuestring);
        if (strstr(type->valuestring, "water-now") != NULL) {
            for (int i = 0; i < (sizeof(connected_devices) / sizeof(connected_devices[0])); i++) {
                if (strstr(connected_devices[i].mac_str, macAddress->valuestring) != NULL) {
                    bzero(connected_devices[i].command, sizeof(connected_devices[i].command));
                    snprintf(connected_devices[i].command, sizeof(connected_devices[i].command), "read sensorp");
                }
            }
        }
    } else {
        ESP_LOGE(TAG, "type is not a valid string\n");
    }

    // Clean up
    cJSON_Delete(json);

    return;
}

void http_json_write(char * http_buffer) {
    // Create the root object
    cJSON *root = cJSON_CreateObject();

    // Create the "plants" array and add MAC addresses
    cJSON *plants_array = cJSON_CreateArray();
    for (int i = 0; i < (sizeof(connected_devices) / sizeof(connected_devices[0])); i++) {
        if ((strstr(connected_devices[i].ipaddr_str, "192.168") != NULL)) {
            cJSON_AddItemToArray(plants_array, cJSON_CreateString(connected_devices[i].mac_str));
        }
    }
    cJSON_AddItemToObject(root, "plants", plants_array);

    // Create the "data" object
    cJSON *data_object = cJSON_CreateObject();
    for (int i = 0; i < (sizeof(connected_devices) / sizeof(connected_devices[0])); i++) {
        // Create the "data" for each plant
        if ((strstr(connected_devices[i].ipaddr_str, "192.168") != NULL)) {
            cJSON *plant_data_object = cJSON_CreateObject();
            cJSON_AddNumberToObject(plant_data_object, "temperature", connected_devices[i].temperature);
            cJSON_AddNumberToObject(plant_data_object, "moisture", connected_devices[i].moisture);
            cJSON_AddNumberToObject(plant_data_object, "uv", connected_devices[i].uv);

            // Add the plant data to the "data" object
            cJSON_AddItemToObject(data_object, connected_devices[i].mac_str, plant_data_object);
        }
    }
    cJSON_AddItemToObject(root, "data", data_object);

    // Add the "water" value
    cJSON_AddNumberToObject(root, "water", 80);

    // Serialize the JSON to a string (buffer)
    char *json_string = cJSON_PrintUnformatted(root);  // formatted string
    // Use cJSON_PrintUnformatted(root) for a compact (non-pretty) version

    if (json_string == NULL) {
        ESP_LOGE(TAG, "Error generating JSON string\n");
        cJSON_Delete(root);
        return;
    }

    // Output the resulting JSON string
    if (strlen(sta_info.identifier) == 0) {
        snprintf(sta_info.identifier, sizeof(sta_info.identifier), "default");
    }

    snprintf(http_buffer, BUFFER_SIZE * 2, "PUT %s/%s HTTP/1.0\r\nHost: %s:%d\r\nContent-Type: application/json; charset=utf-8\r\nContent-Length: %d\r\n\r\n%s", WEB_PATH, sta_info.identifier, WEB_SERVER, WEB_PORT, strlen(json_string), json_string);
    
    // Free the generated string and JSON object
    free(json_string);
    cJSON_Delete(root);

    return;
}

// static void http_put_task(void *pvParameters)
// {
//     /* DNS Look up */
//     // const struct addrinfo hints = {
//     //     .ai_family = AF_INET,
//     //     .ai_socktype = SOCK_STREAM,
//     // };
//     // struct addrinfo *res;
//     // struct in_addr *addr;
//     int s, r;
//     char recv_buf[BUFFER_SIZE];
//     char send_buf[BUFFER_SIZE];

//     /* Direct IP */
//     int addr_family = 0;
//     int ip_protocol = 0;

//     while(1) {
//         struct sockaddr_in dest_addr;
//         dest_addr.sin_addr.s_addr = inet_addr(WEB_SERVER);
//         dest_addr.sin_family = AF_INET;
//         dest_addr.sin_port = htons(WEB_PORT);
//         addr_family = AF_INET;
//         ip_protocol = IPPROTO_IP;

//         s = socket(addr_family, SOCK_STREAM, ip_protocol);
//         if(s < 0) {
//             ESP_LOGE(TAG, "... Failed to allocate socket.");
//             vTaskDelay(1000 / portTICK_PERIOD_MS);
//             continue;
//         }
//         else {
//             // Set timeout
//             struct timeval timeout;
//             timeout.tv_sec = 5;
//             timeout.tv_usec = 0;
//             setsockopt (s, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof timeout);
//         }
//         ESP_LOGI(TAG, "... allocated socket");

//         if(connect(s, (struct sockaddr *)&dest_addr, INET_ADDRSTRLEN) != 0) {
//             ESP_LOGE(TAG, "... socket connect failed errno=%d", errno);
//             close(s);
//             vTaskDelay(4000 / portTICK_PERIOD_MS);
//             continue;
//         }

//         ESP_LOGI(TAG, "... connected");

//         http_json_write(send_buf);

//         if (write(s, send_buf, strlen(send_buf)) < 0) {
//             ESP_LOGE(TAG, "... socket send failed");
//             close(s);
//             vTaskDelay(4000 / portTICK_PERIOD_MS);
//             continue;
//         }
//         ESP_LOGI(TAG, "... socket send success");

//         // struct timeval receiving_timeout;
//         // receiving_timeout.tv_sec = 5;
//         // receiving_timeout.tv_usec = 0;
//         // if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &receiving_timeout,
//         //         sizeof(receiving_timeout)) < 0) {
//         //     ESP_LOGE(TAG, "... failed to set socket receiving timeout");
//         //     close(s);
//         //     vTaskDelay(4000 / portTICK_PERIOD_MS);
//         //     continue;
//         // }
//         // ESP_LOGI(TAG, "... set socket receiving timeout success");

//         /* Read HTTP response */
//         do {
//             bzero(recv_buf, sizeof(recv_buf));
//             r = read(s, recv_buf, sizeof(recv_buf)-1);
//             for(int i = 0; i < r; i++) {
//                 putchar(recv_buf[i]);
//             }
//         } while(r > 0);

//         // char * content_ptr;
//         // content_ptr = strchr(recv_buf, '{');
//         http_json_read(recv_buf);

//         ESP_LOGI(TAG, "... done reading from socket. Last read return=%d errno=%d.", r, errno);
//         close(s);
//         // for(int countdown = 10; countdown >= 0; countdown--) {
//         //     ESP_LOGI(TAG, "%d... ", countdown);
//         //     vTaskDelay(1000 / portTICK_PERIOD_MS);
//         // }
//         vTaskDelay(10000 / portTICK_PERIOD_MS);
//         ESP_LOGI(TAG, "Starting again!");

//         // int err = getaddrinfo(WEB_SERVER, WEB_PORT, &hints, &res);

//         // if(err != 0 || res == NULL) {
//         //     ESP_LOGE(TAG, "DNS lookup failed err=%d res=%p", err, res);
//         //     vTaskDelay(1000 / portTICK_PERIOD_MS);
//         //     continue;
//         // }

//         // /* Code to print the resolved IP.

//         //    Note: inet_ntoa is non-reentrant, look at ipaddr_ntoa_r for "real" code */
//         // addr = &((struct sockaddr_in *)res->ai_addr)->sin_addr;
//         // ESP_LOGI(TAG, "DNS lookup succeeded. IP=%s", inet_ntoa(*addr));

//         // s = socket(res->ai_family, res->ai_socktype, 0);
//         // if(s < 0) {
//         //     ESP_LOGE(TAG, "... Failed to allocate socket.");
//         //     freeaddrinfo(res);
//         //     vTaskDelay(1000 / portTICK_PERIOD_MS);
//         //     continue;
//         // }
//         // ESP_LOGI(TAG, "... allocated socket");

//         // if(connect(s, res->ai_addr, res->ai_addrlen) != 0) {
//         //     ESP_LOGE(TAG, "... socket connect failed errno=%d", errno);
//         //     close(s);
//         //     freeaddrinfo(res);
//         //     vTaskDelay(4000 / portTICK_PERIOD_MS);
//         //     continue;
//         // }

//         // ESP_LOGI(TAG, "... connected");
//         // freeaddrinfo(res);

//         // if (write(s, REQUEST, strlen(REQUEST)) < 0) {
//         //     ESP_LOGE(TAG, "... socket send failed");
//         //     close(s);
//         //     vTaskDelay(4000 / portTICK_PERIOD_MS);
//         //     continue;
//         // }
//         // ESP_LOGI(TAG, "... socket send success");

//         // struct timeval receiving_timeout;
//         // receiving_timeout.tv_sec = 5;
//         // receiving_timeout.tv_usec = 0;
//         // if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &receiving_timeout,
//         //         sizeof(receiving_timeout)) < 0) {
//         //     ESP_LOGE(TAG, "... failed to set socket receiving timeout");
//         //     close(s);
//         //     vTaskDelay(4000 / portTICK_PERIOD_MS);
//         //     continue;
//         // }
//         // ESP_LOGI(TAG, "... set socket receiving timeout success");

//         // /* Read HTTP response */
//         // do {
//         //     bzero(recv_buf, sizeof(recv_buf));
//         //     r = read(s, recv_buf, sizeof(recv_buf)-1);
//         //     for(int i = 0; i < r; i++) {
//         //         putchar(recv_buf[i]);
//         //     }
//         // } while(r > 0);

//         // ESP_LOGI(TAG, "... done reading from socket. Last read return=%d errno=%d.", r, errno);
//         // close(s);
//         // for(int countdown = 10; countdown >= 0; countdown--) {
//         //     ESP_LOGI(TAG, "%d... ", countdown);
//         //     vTaskDelay(1000 / portTICK_PERIOD_MS);
//         // }
//         // ESP_LOGI(TAG, "Starting again!");
//     }
// }

// void http_client_init(void)
// {
//     // ESP_ERROR_CHECK( nvs_flash_init() );
//     // ESP_ERROR_CHECK(esp_netif_init());
//     // ESP_ERROR_CHECK(esp_event_loop_create_default());

//     /* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
//      * Read "Establishing Wi-Fi or Ethernet Connection" section in
//      * examples/protocols/README.md for more information about this function.
//      */
//     // ESP_ERROR_CHECK(example_connect());

//     xTaskCreate(&http_put_task, "http_put_task", 1024 * 16, NULL, 5, NULL);
// }

void http_put(void)
{
    /* DNS Look up */
    // const struct addrinfo hints = {
    //     .ai_family = AF_INET,
    //     .ai_socktype = SOCK_STREAM,
    // };
    // struct addrinfo *res;
    // struct in_addr *addr;
    int s, r;
    char recv_buf[BUFFER_SIZE];
    char content_buf[BUFFER_SIZE * 2];
    char send_buf[BUFFER_SIZE * 2];

    bzero(content_buf, sizeof(content_buf));

    /* Direct IP */
    int addr_family = 0;
    int ip_protocol = 0;

    struct sockaddr_in dest_addr;
    dest_addr.sin_addr.s_addr = inet_addr(WEB_SERVER);
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(WEB_PORT);
    addr_family = AF_INET;
    ip_protocol = IPPROTO_IP;

    s = socket(addr_family, SOCK_STREAM, ip_protocol);
    if(s < 0) {
        ESP_LOGE(TAG, "... Failed to allocate socket.");
        // vTaskDelay(1000 / portTICK_PERIOD_MS);
        return;
    }

    // Set timeout
    struct timeval timeout;
    timeout.tv_sec = 10;
    timeout.tv_usec = 0;
    // setsockopt (s, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof timeout);
    setsockopt (s, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof timeout);

    ESP_LOGI(TAG, "... allocated socket");

    if(connect(s, (struct sockaddr *)&dest_addr, INET_ADDRSTRLEN) != 0) {
        ESP_LOGE(TAG, "... socket connect failed errno=%d", errno);
        close(s);
        // vTaskDelay(4000 / portTICK_PERIOD_MS);
        return;
    }

    ESP_LOGI(TAG, "... connected");

    http_json_write(send_buf);
    printf("%s\n", send_buf);

    int bytes;
    if ((bytes = write(s, send_buf, strlen(send_buf))) < 0) {
        ESP_LOGE(TAG, "... socket send failed");
        close(s);
        // vTaskDelay(4000 / portTICK_PERIOD_MS);
        return;
    }
    ESP_LOGI(TAG, "... socket send success, bytes written: %d", bytes);

    // struct timeval receiving_timeout;
    // receiving_timeout.tv_sec = 5;
    // receiving_timeout.tv_usec = 0;
    // if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &receiving_timeout,
    //         sizeof(receiving_timeout)) < 0) {
    //     ESP_LOGE(TAG, "... failed to set socket receiving timeout");
    //     close(s);
    //     vTaskDelay(4000 / portTICK_PERIOD_MS);
    //     continue;
    // }
    // ESP_LOGI(TAG, "... set socket receiving timeout success");
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;
    setsockopt (s, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof timeout);

    /* Read HTTP response */
    do {
        bzero(recv_buf, sizeof(recv_buf));
        r = read(s, recv_buf, sizeof(recv_buf)-1);
        strcat(content_buf, recv_buf); // only works since we're expecting json
        for(int i = 0; i < r; i++) {
            putchar(recv_buf[i]);
        }
    } while(r > 0);

    // printf("HTTP Receive Buffer: %s\n", recv_buf);

    char * content_ptr;
    if ((content_ptr = strstr(content_buf, "\r\n\r\n")) != NULL) {
        // printf("\n%s\n", content_ptr);
        http_json_read(content_ptr);
    }

    ESP_LOGI(TAG, "... done reading from socket. Last read return=%d errno=%d.", r, errno);
    close(s);
    // for(int countdown = 10; countdown >= 0; countdown--) {
    //     ESP_LOGI(TAG, "%d... ", countdown);
    //     vTaskDelay(1000 / portTICK_PERIOD_MS);
    // }
}