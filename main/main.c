/**
 * @file    main.c
 * @brief   Application entry point and task scheduler
 *
 * Task layout (by timing requirement):
 *
 *   task_100ms   — fast digital I/O: obstacle, IR motion, light control timer
 *   task_2000ms  — slow sensors: DHT11, light intensity, MQTT publish
 *   task_startup — one-shot: starts MQTT client, then self-deletes
 *
 * Each task only calls the periodic run() functions of the relevant modules.
 * No sensor logic lives in this file.
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs_flash.h"

/* Module headers */
#include "wifi_sta.h"
#include "led.h"
#include "relay.h"
#include "light_ctrl.h"
#include "obstacle.h"
#include "ir_sensor.h"
#include "dht11.h"
#include "light_sensor.h"
#include "mqtt_manager.h"
#include "esp_adc/adc_oneshot.h"

static const char *TAG = "main";

/* ── Pin definitions ─────────────────────────────────────────────────────── */
#define LED_GPIO            GPIO_NUM_2
#define RELAY_GPIO          GPIO_NUM_15
#define OBSTACLE_GPIO       GPIO_NUM_4
#define IR_SENSOR_GPIO      GPIO_NUM_23
#define DHT11_GPIO          GPIO_NUM_19
#define LIGHT_DIGITAL_GPIO  GPIO_NUM_13
#define LIGHT_ADC_CHANNEL   ADC_CHANNEL_6   /* GPIO34 */


/* ════════════════════════════════════════════════════════════════════════════
 * task_100ms — fast digital I/O (100 ms period)
 *
 *   obstacle_run()    read door/window sensor → sensor_state
 *   ir_sensor_run()   read PIR → sensor_state + notify light_ctrl
 *   light_ctrl_run()  check auto-off countdown timer
 * ════════════════════════════════════════════════════════════════════════════ */
static void task_100ms(void *arg)
{
    ESP_LOGI(TAG, "task_100ms started");
    while (1) {
        obstacle_run();
        ir_sensor_run();
        light_ctrl_run();
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}


/* ════════════════════════════════════════════════════════════════════════════
 * task_2000ms — slow sensors + network publish (2000 ms period)
 *
 *   dht11_run()         read temperature & humidity → sensor_state
 *   light_sensor_run()  read lux → sensor_state
 *   mqtt_manager_run()  publish all sensor_state values to MQTT broker
 * ════════════════════════════════════════════════════════════════════════════ */
static void task_2000ms(void *arg)
{
    ESP_LOGI(TAG, "task_2000ms started");
    while (1) {
        dht11_run();
        light_sensor_run();
        mqtt_manager_run();
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}


/* ════════════════════════════════════════════════════════════════════════════
 * task_startup — one-shot network initialisation
 *
 *   Starts the MQTT client (connects to broker asynchronously), then
 *   self-deletes.  Runs at higher priority so it completes before periodic
 *   tasks begin publishing.
 * ════════════════════════════════════════════════════════════════════════════ */
static void task_startup(void *arg)
{
    if (mqtt_manager_init(NULL) != ESP_OK) {
        ESP_LOGE(TAG, "mqtt_manager_init failed");
    }
    vTaskDelete(NULL);
}


/* ════════════════════════════════════════════════════════════════════════════
 * app_main — system entry point
 * ════════════════════════════════════════════════════════════════════════════ */
void app_main(void)
{
    /* ── NVS (required by WiFi) ── */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    /* ── WiFi — continues even on timeout ── */
    wifi_station_startup();

    /* ── Hardware module initialisation ── */
    led_init(LED_GPIO);
    relay_init(RELAY_GPIO);
    light_ctrl_init();
    obstacle_init(OBSTACLE_GPIO);
    ir_sensor_init(IR_SENSOR_GPIO);
    esp_log_level_set("dht11", ESP_LOG_NONE);
    dht11_init(DHT11_GPIO);
    light_sensor_init(LIGHT_DIGITAL_GPIO, LIGHT_ADC_CHANNEL);

    /* Startup blink: 3 × 200 ms, then LED stays on as system-running indicator */
    for (int i = 0; i < 3; i++) {
        led_on();  vTaskDelay(pdMS_TO_TICKS(200));
        led_off(); vTaskDelay(pdMS_TO_TICKS(200));
    }
    led_on();

    /* ── Create tasks ── */
    xTaskCreate(task_startup, "task_startup",  4096, NULL, 5, NULL);
    xTaskCreate(task_100ms,   "task_100ms",    4096, NULL, 4, NULL);
    xTaskCreate(task_2000ms,  "task_2000ms",   4096, NULL, 3, NULL);
}
