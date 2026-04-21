/**
 * @file    obstacle.c
 * @brief   Obstacle detection module implementation
 *
 * The module's OUT pin is open-drain output, active low:
 *   Obstacle detected → OUT pulled low  → GPIO reads 0
 *   No obstacle       → OUT released    → pull-up pulls high → GPIO reads 1
 */

#include "obstacle.h"
#include "esp_log.h"

static const char *TAG = "obstacle";

/* Stores the GPIO number configured at initialization */
static gpio_num_t s_gpio_num = GPIO_NUM_NC;


/**
 * @brief  Initialize the obstacle sensor; configure GPIO as pull-up input
 *
 * @param gpio_num  GPIO number connected to the OUT pin
 * @return ESP_OK / ESP_FAIL
 */
esp_err_t obstacle_init(gpio_num_t gpio_num)
{
    s_gpio_num = gpio_num;

    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << gpio_num),
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_ENABLE,   /* pull-up, keeps high when no obstacle */
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };

    esp_err_t ret = gpio_config(&io_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "gpio_config failed: %d", ret);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "obstacle sensor init ok, gpio=%d", gpio_num);
    return ESP_OK;
}


/**
 * @brief  Read the current detection state
 *
 * @return 1 = obstacle detected (OUT=LOW), 0 = no obstacle (OUT=HIGH)
 */
int obstacle_detected(void)
{
    /* OUT is active low; invert so that 1 means "detected" */
    return gpio_get_level(s_gpio_num) == 0 ? 1 : 0;
}
