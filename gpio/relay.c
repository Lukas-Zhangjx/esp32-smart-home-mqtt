/**
 * @file    relay.c
 * @brief   Relay / GPIO output control module implementation
 *
 * Push-pull output mode, active high.
 * Currently supports a single output channel (GPIO15);
 * extend to an array for multi-channel support.
 */

#include "relay.h"
#include "esp_log.h"

static const char *TAG = "relay";

/* Stores the GPIO number configured at initialization */
static gpio_num_t s_gpio_num = GPIO_NUM_NC;

/* Cached current state: 1 = on (conducting), 0 = off (open) */
static int s_state = 0;


/**
 * @brief  Initialize the relay GPIO; configure as push-pull output, off by default
 *
 * @param gpio_num  GPIO number connected to the relay/LED
 * @return ESP_OK / ESP_FAIL
 */
esp_err_t relay_init(gpio_num_t gpio_num)
{
    s_gpio_num = gpio_num;

    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << gpio_num),
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };

    esp_err_t ret = gpio_config(&io_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "gpio_config failed: %d", ret);
        return ESP_FAIL;
    }

    /* Default to off after initialization */
    gpio_set_level(s_gpio_num, 0);
    s_state = 0;

    ESP_LOGI(TAG, "relay init ok, gpio=%d", gpio_num);
    return ESP_OK;
}


/**
 * @brief  Set the relay state
 *
 * @param state  1 = on (conducting), 0 = off (open)
 * @return       The actual state after being set
 */
int relay_set(int state)
{
    s_state = (state != 0) ? 1 : 0;
    gpio_set_level(s_gpio_num, s_state);
    ESP_LOGI(TAG, "relay -> %s", s_state ? "ON" : "OFF");
    return s_state;
}


/**
 * @brief  Get the current relay state
 *
 * @return 1 = on (conducting), 0 = off (open)
 */
int relay_get_state(void)
{
    return s_state;
}
