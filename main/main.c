#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "wifi_sta.h"
#include "http_server.h"
#include "dht11.h"
#include "led.h"
#include "relay.h"
#include "obstacle.h"
#include "light_ctrl.h"
#include "ir_sensor.h"
#include "light_sensor.h"
#include "esp_adc/adc_oneshot.h"

static const char *TAG = "main";

/* Pin definitions */
#define DHT11_GPIO    GPIO_NUM_19
#define LED_GPIO      GPIO_NUM_2
#define OBSTACLE_GPIO  GPIO_NUM_4
#define IR_SENSOR_GPIO GPIO_NUM_23
#define RELAY_GPIO          GPIO_NUM_15
#define LIGHT_DIGITAL_GPIO  GPIO_NUM_13
#define LIGHT_ADC_CHANNEL   ADC_CHANNEL_6   /* GPIO34 */


/* ================================================================
 * io_task: GPIO digital input module
 *   Responsibility: read all digital switch inputs, e.g. obstacle
 *                   detection, buttons, etc.
 *   Period: 100 ms
 * ================================================================ */
static void io_task(void *pvParameters)
{
    int last_obstacle = -1; /* previous obstacle state, -1 = uninitialized */
    int last_ir       = -1; /* previous IR state, -1 = uninitialized */

    ESP_LOGI(TAG, "io_task started");

    /* --- main loop --- */
    while (1) {
        /* Door/window sensor (obstacle module, GPIO22) */
        int obstacle = obstacle_detected();
        if (obstacle != last_obstacle) {
            ESP_LOGI(TAG, "door: %s", obstacle ? "CLOSED" : "OPEN");
            http_server_update_obstacle(obstacle);
            last_obstacle = obstacle;
        }

        /* IR detection sensor (GPIO23) */
        int ir = ir_sensor_detected();
        if (ir != last_ir) {
            ESP_LOGI(TAG, "ir: %s", ir ? "DETECTED" : "clear");
            http_server_update_ir(ir);
            last_ir = ir;
        }

        /* Notify the light control module of the PIR state */
        if (ir) {
            light_ctrl_on_motion();
        } else {
            light_ctrl_on_idle();
        }

        /* Check the auto-off countdown timer */
        light_ctrl_tick();

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}


/* ================================================================
 * sensor_task: analog / bus sensor module
 *   Responsibility: read all sensors and update the data cache
 *                   for use by network_task
 *   Period: 2000 ms (limited by DHT11 sampling rate)
 * ================================================================ */
static void sensor_task(void *pvParameters)
{
    ESP_LOGI(TAG, "sensor_task started");

    /* --- main loop --- */
    while (1) {
        /* DHT11 temperature and humidity */
        http_server_update_sensor();

        /* Light sensor: analog + digital */
        int raw     = light_sensor_analog();
        int percent = light_sensor_to_percent(raw);
        int bright  = light_sensor_digital();
        http_server_update_light(percent, raw, bright);

        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}


/* ================================================================
 * network_task: network service module
 *   Responsibility: start and maintain all network services,
 *                   e.g. HTTP Server, MQTT, etc.
 * ================================================================ */
static void network_task(void *pvParameters)
{
    /* --- module initialization --- */
    if (http_server_start() != ESP_OK) {
        ESP_LOGE(TAG, "http server failed to start");
    }

    /* HTTP Server is driven internally by esp_http_server; this task needs no main loop */
    vTaskDelete(NULL);
}


/* ================================================================
 * output_task: output module
 *   Responsibility: drive all output devices, e.g. LED, OLED, etc.
 *   Period: defined by the requirements of each output device
 * ================================================================ */
static void output_task(void *pvParameters)
{
    ESP_LOGI(TAG, "output_task started");

    /* Blink GPIO2 three times */
    for (int i = 0; i < 3; i++) {
        led_on();
        vTaskDelay(pdMS_TO_TICKS(200));
        led_off();
        vTaskDelay(pdMS_TO_TICKS(200));
    }


    /* System running indicator: stay on */
    led_on();

    /* --- main loop --- */
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}


/* ================================================================
 * app_main: system entry point
 *   Responsibility: initialize system-level components and create
 *                   all framework tasks
 * ================================================================ */
void app_main(void)
{
    /* Initialize NVS */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    /* Connect to WiFi; continue running even if it times out */
    wifi_station_startup();

    /* Hardware module initialization (done before tasks start to avoid race conditions) */
    led_init(LED_GPIO);
    relay_init(RELAY_GPIO);
    light_ctrl_init();
    obstacle_init(OBSTACLE_GPIO);
    ir_sensor_init(IR_SENSOR_GPIO);
    esp_log_level_set("dht11", ESP_LOG_NONE);
    dht11_init(DHT11_GPIO);
    light_sensor_init(LIGHT_DIGITAL_GPIO, LIGHT_ADC_CHANNEL);

    /* Create framework tasks */
    xTaskCreate(io_task,      "io_task",      4096, NULL, 4, NULL);
    xTaskCreate(sensor_task,  "sensor_task",  4096, NULL, 4, NULL);
    xTaskCreate(network_task, "network_task", 4096, NULL, 5, NULL);
    xTaskCreate(output_task,  "output_task",  4096, NULL, 3, NULL);
}
