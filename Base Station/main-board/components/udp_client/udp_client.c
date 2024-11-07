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

#define PORT CONFIG_UDP_PORT
#define HOST_IP_ADDR "192.168.4.2"

static const char *TAG = "UDP Client";
static const char *payload = "read sensors";

static void udp_client_task(void *pvParameters)
{
    char rx_buffer[2048];
    char host_ip[] = HOST_IP_ADDR;
    int addr_family = 0;
    int ip_protocol = 0;

    while (1) {

        struct sockaddr_in dest_addr;
        dest_addr.sin_addr.s_addr = inet_addr(HOST_IP_ADDR);
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
        

            while (1) {
                ESP_LOGI(TAG, "Sending command");
                int err = sendto(sock, payload, strlen(payload), 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));

                // Error occurred during sending
                if (err < 0) {
                    ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
                    break;
                }
                ESP_LOGI(TAG, "Message sent");
                struct sockaddr_storage source_addr; // Large enough for both IPv4 or IPv6
                socklen_t socklen = sizeof(source_addr);
                int len = recvfrom(sock, rx_buffer, sizeof(rx_buffer) - 1, 0, (struct sockaddr *)&source_addr, &socklen);
                // Error occurred during receiving
                if (len < 0) {
                    ESP_LOGE(TAG, "recvfrom failed: errno %d", errno);
                    break;
                }
                // Data received
                else {
                    rx_buffer[len] = 0; // Null-terminate whatever we received and treat like a string
                    ESP_LOGI(TAG, "Received %d bytes from %s:", len, host_ip);
                    ESP_LOGI(TAG, "%s", rx_buffer);
                    len = recvfrom(sock, rx_buffer, sizeof(rx_buffer) - 1, 0, (struct sockaddr *)&source_addr, &socklen);
                    rx_buffer[len] = 0;
                    while ((strstr(rx_buffer, "Sensors reading complete.")) != NULL) {
                        ESP_LOGI(TAG, "%s", rx_buffer);
                        len = recvfrom(sock, rx_buffer, sizeof(rx_buffer) - 1, 0, (struct sockaddr *)&source_addr, &socklen);
                        rx_buffer[len] = 0;
                    }
                }
            }
            vTaskDelay(pdMS_TO_TICKS(5000));
        }
        if (sock != -1) {
            ESP_LOGE(TAG, "Shutting down socket and restarting...");
            shutdown(sock, 0);
            close(sock);
        }
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
    // vTaskDelete(NULL);
}

void udp_client_init(void)
{
    // ESP_ERROR_CHECK(nvs_flash_init());
    // ESP_ERROR_CHECK(esp_netif_init());
    // ESP_ERROR_CHECK(esp_event_loop_create_default());

    xTaskCreate(udp_client_task, "udp_server", 4096*2, NULL, 5, NULL);

}