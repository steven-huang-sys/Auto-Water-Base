/* GPIO Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "hub_controller.h"
#include "esp_timer.h"

#define GPIO_INPUT_IO_0     3
#define GPIO_INPUT_IO_1     4
#define GPIO_INPUT_IO_2     5
#define GPIO_INPUT_IO_3     6
#define GPIO_INPUT_IO_4     7
#define GPIO_INPUT_PIN_SEL  ((1ULL<<GPIO_INPUT_IO_0) | (1ULL<<GPIO_INPUT_IO_1) | (1ULL<<GPIO_INPUT_IO_2) | (1ULL<<GPIO_INPUT_IO_3) | (1ULL<<GPIO_INPUT_IO_4))
/*
 * Let's say, GPIO_INPUT_IO_0=4, GPIO_INPUT_IO_1=5
 * In binary representation,
 * 1ULL<<GPIO_INPUT_IO_0 is equal to 0000000000000000000000000000000000010000 and
 * 1ULL<<GPIO_INPUT_IO_1 is equal to 0000000000000000000000000000000000100000
 * GPIO_INPUT_PIN_SEL                0000000000000000000000000000000000110000
 * */
#define ESP_INTR_FLAG_DEFAULT 0


// timer config

esp_timer_handle_t timer_handler;

static void timer_callback(void *param) {
    // delete timer
    esp_timer_delete(timer_handler);
    timer_handler = NULL;
}

const esp_timer_create_args_t timer_config = {
    .callback = &timer_callback,
    .name = "Timer"
};

// gpio handlers
static QueueHandle_t gpio_evt_queue = NULL;

static void IRAM_ATTR gpio_isr_handler(void* arg)
{
    if (timer_handler != NULL) {
        return;
    }
    ESP_ERROR_CHECK(esp_timer_create(&timer_config, &timer_handler));
    ESP_ERROR_CHECK(esp_timer_start_once(timer_handler, 200000));
    uint32_t gpio_num = (uint32_t) arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

static void gpio_task_example(void* arg)
{
    uint32_t io_num;
    for (;;) {
        if (xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)) {
            printf("GPIO[%"PRIu32"] intr, val: %d\n", io_num, gpio_get_level(io_num));
            // io_num = 0;
        }  
    }
}

/*
static void gpio_polling(void* arg)
{
    for (;;) {
        if (gpio_get_level(GPIO_INPUT_IO_0)) {
            printf("GPIO[%d] pressed\n", GPIO_INPUT_IO_0);
        }
        else if (gpio_get_level(GPIO_INPUT_IO_1)) {
            printf("GPIO[%d] pressed\n", GPIO_INPUT_IO_1);
        }
        else if (gpio_get_level(GPIO_INPUT_IO_2)) {
            printf("GPIO[%d] pressed\n", GPIO_INPUT_IO_2);
        }
        else if (gpio_get_level(GPIO_INPUT_IO_3)) {
            printf("GPIO[%d] pressed\n", GPIO_INPUT_IO_3);
        }
        else if (gpio_get_level(GPIO_INPUT_IO_4)) {
            printf("GPIO[%d] pressed\n", GPIO_INPUT_IO_4);
        }
        vTaskDelay(200 / portTICK_PERIOD_MS);
    }
}
*/

void control_init(void)
{
    //zero-initialize the config structure.
    gpio_config_t io_conf = {};

    printf("Input config\n");
    //interrupt edge type
    io_conf.intr_type = GPIO_INTR_POSEDGE;
    //bit mask of the pins
    io_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL;
    //set as input mode
    io_conf.mode = GPIO_MODE_INPUT;
    //pull-down mode
    io_conf.pull_down_en = 1;
    //pull-up mode
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);


    //change gpio interrupt type for one pin
    // gpio_set_intr_type(GPIO_INPUT_IO_0, GPIO_INTR_ANYEDGE);
    printf("Handler initialization\n");
    //create a queue to handle gpio event from isr
    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));
    //start gpio task
    xTaskCreate(gpio_task_example, "gpio_task_example", 2048, NULL, 10, NULL);

    //install gpio isr service
    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    //hook isr handler for specific gpio pin
    gpio_isr_handler_add(GPIO_INPUT_IO_0, gpio_isr_handler, (void*) GPIO_INPUT_IO_0);
    //hook isr handler for specific gpio pin
    gpio_isr_handler_add(GPIO_INPUT_IO_1, gpio_isr_handler, (void*) GPIO_INPUT_IO_1);
    gpio_isr_handler_add(GPIO_INPUT_IO_2, gpio_isr_handler, (void*) GPIO_INPUT_IO_2);
    gpio_isr_handler_add(GPIO_INPUT_IO_3, gpio_isr_handler, (void*) GPIO_INPUT_IO_3);
    gpio_isr_handler_add(GPIO_INPUT_IO_4, gpio_isr_handler, (void*) GPIO_INPUT_IO_4);
    

    // gpio polling
    // xTaskCreate(gpio_polling, "gpio_polling", 2048, NULL, 10, NULL);

    printf("Minimum free heap size: %"PRIu32" bytes\n", esp_get_minimum_free_heap_size());

}
