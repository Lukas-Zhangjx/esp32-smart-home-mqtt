/**
 * @file    ir_sensor.c
 * @brief   IR detection module implementation
 *
 * Suitable for IR sensor modules with digital output (e.g. FC-51, TCRT5000, etc.).
 * The module's OUT pin is active low:
 *   Target detected → OUT pulled low  → GPIO reads 0
 *   No target       → OUT released    → pull-up pulls high → GPIO reads 1
 */

#include "ir_sensor.h"
#include "esp_log.h"

static const char *TAG = "ir_sensor";

/* Stores the GPIO number configured at initialization */
static gpio_num_t s_gpio_num = GPIO_NUM_NC;


/**
 * @brief  Initialize the IR sensor; configure GPIO as pull-up input
 *
 * @param gpio_num  GPIO number connected to the OUT pin
 * @return ESP_OK / ESP_FAIL
 */
esp_err_t ir_sensor_init(gpio_num_t gpio_num)
{
    s_gpio_num = gpio_num;

    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << gpio_num),
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,   /* HC-SR501 is self-driven, no pull-up needed */
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };

    esp_err_t ret = gpio_config(&io_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "gpio_config failed: %d", ret);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "ir sensor init ok, gpio=%d", gpio_num);
    return ESP_OK;
}


/**
 * @brief  Read the current detection state
 *
 * @return 1 = target detected (OUT=LOW), 0 = no target (OUT=HIGH)
 */
int ir_sensor_detected(void)
{
    /* HC-SR501 is active high: OUT=HIGH means motion detected */
    return gpio_get_level(s_gpio_num);
}
