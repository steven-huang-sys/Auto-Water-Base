#include <string.h>
#include <sys/param.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
// #include "nvs_flash.h"
#include "esp_netif.h"
// #include "protocol_examples_common.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>

#include "shared_variables.h"
// #include "../http_client/http_client.h"
#include "http_client.h"

#define PORT CONFIG_UDP_PORT
#define HOST_IP_ADDR "192.168.4.2"

static const char *TAG = "UDP Client";
// static const char *payload = "read sensors";

void udp_client_task(void *pvParameters)
{
    char rx_buffer[1024];
    // char host_ip[INET_ADDRSTRLEN];
    int addr_family = 0;
    int ip_protocol = 0;

    while (1) {

        for (int i = 0; i < sizeof(connected_devices[0]); i++) {
            if ((strstr(connected_devices[i].ipaddr_str, "192.168") != NULL)) {
                ESP_LOGI(TAG, "Attempt to create socket with %s", connected_devices[i].ipaddr_str);
                if (strlen(connected_devices[i].command) == 0) {
                    snprintf(connected_devices[i].command, sizeof(connected_devices[i].command), "read sensors");
                }

                struct sockaddr_in dest_addr;
                dest_addr.sin_addr.s_addr = inet_addr(connected_devices[i].ipaddr_str);
                dest_addr.sin_family = AF_INET;
                dest_addr.sin_port = htons(PORT);
                addr_family = AF_INET;
                ip_protocol = IPPROTO_IP;

                int sock = socket(addr_family, SOCK_DGRAM, ip_protocol);
                if (sock < 0) {
                    ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
                }
                else {
                    ESP_LOGI(TAG, "Socket created");

                    // Set timeout
                    struct timeval timeout;
                    timeout.tv_sec = 10;
                    timeout.tv_usec = 0;
                    setsockopt (sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof timeout);
                
                    int err = 0;
                    if (xSemaphoreTake(bufferMutex, portMAX_DELAY)) {
                        ESP_LOGI(TAG, "Sending command: %s", connected_devices[i].command);
                        err = sendto(sock, connected_devices[i].command, strlen(connected_devices[i].command), 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
                        // err = sendto(sock, ble_in, ble_numbytes, 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));

                        // Error occurred during sending
                        if (err < 0) {
                            ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
                        }
                        xSemaphoreGive(bufferMutex);
                    }

                    if (err >= 0) {
                        ESP_LOGI(TAG, "Message sent");
                        struct sockaddr_storage source_addr; // Large enough for both IPv4 or IPv6
                        socklen_t socklen = sizeof(source_addr);
                        int len;
                        do {
                            len = recvfrom(sock, rx_buffer, sizeof(rx_buffer) - 1, 0, (struct sockaddr *)&source_addr, &socklen);
                            // Error occurred during receiving
                            if (len < 0) {
                                ESP_LOGE(TAG, "recvfrom failed: errno %d", errno);
                            }
                            // Data received
                            rx_buffer[len] = 0; // Null-terminate whatever we received and treat like a string
                            ESP_LOGI(TAG, "Received %d bytes from %s:", len, connected_devices[i].ipaddr_str);
                            ESP_LOGI(TAG, "%s", rx_buffer);
                            if (strstr(connected_devices[i].command, "pump") == NULL) {
                                char * buff_ptr;
                                if ((buff_ptr = strstr(rx_buffer, "Moisture")) != NULL) {
                                    sscanf(buff_ptr, "Moisture: %d", &connected_devices[i].moisture);
                                }
                                if ((buff_ptr = strstr(rx_buffer, "Temperature")) != NULL) {
                                    sscanf(buff_ptr, "Temperature: %f", &connected_devices[i].temperature);
                                }
                                if ((buff_ptr = strstr(rx_buffer, "UV Sensor")) != NULL) {
                                    sscanf(buff_ptr, "UV Sensor: %d", &connected_devices[i].uv);
                                }
                            }
                        } while (((strstr(rx_buffer, "Command Complete")) == NULL) && (len >= 0));
                                
                        bzero(connected_devices[i].command, sizeof(connected_devices[i].command));        
                        snprintf(connected_devices[i].command, sizeof(connected_devices[i].command), "read sensors");
                    }
                    
                }
                if (sock < 0) {
                    ESP_LOGE(TAG, "Socket error, trying next address.");
                }
                else {
                    ESP_LOGI(TAG, "Message transaction complete, moving to next address.");
                }
                shutdown(sock, 0);
                close(sock);
                vTaskDelay(pdMS_TO_TICKS(500));
            }
        }

        http_put();
        // Stack usage monitoring
        UBaseType_t high_water_mark = uxTaskGetStackHighWaterMark(NULL);
        ESP_LOGI(TAG, "Stack high water mark: %d words", high_water_mark);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void udp_client_init(void)
{
    // ESP_ERROR_CHECK(nvs_flash_init());
    // ESP_ERROR_CHECK(esp_netif_init());
    // ESP_ERROR_CHECK(esp_event_loop_create_default());

    ESP_LOGI(TAG, "Creating UDP Client...");
    xTaskCreate(udp_client_task, "udp_client", 1024*30, NULL, 4, NULL);
    ESP_LOGI(TAG, "UDP Client Started");

}