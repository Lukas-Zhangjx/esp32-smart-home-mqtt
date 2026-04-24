/**
 * @file    led.c
 * @brief   LED GPIO control module implementation
 *
 * Push-pull output mode: GPIO high = LED on, GPIO low = LED off.
 * The current state is maintained internally to avoid reading the GPIO register every time.
 */

#include "led.h"
#include "esp_log.h"

static const char *TAG = "led";

/* Stores the GPIO number configured at initialization */
static gpio_num_t s_gpio_num = GPIO_NUM_NC;

/* Cached current LED state: 1 = on, 0 = off */
static int s_state = 0;


/**
 * @brief  Initialize the LED GPIO; configure as push-pull output, off by default
 *
 * @param gpio_num  GPIO number connected to the LED
 * @return ESP_OK / ESP_FAIL
 */
esp_err_t led_init(gpio_num_t gpio_num)
{
    s_gpio_num = gpio_num;

    /* Configure as push-pull output; no pull-up/pull-down needed */
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

    ESP_LOGI(TAG, "led init ok, gpio=%d", gpio_num);
    return ESP_OK;
}

/**
 * @brief  Turn the LED on
 */
void led_on(void)
{
    gpio_set_level(s_gpio_num, 1);
    s_state = 1;
    ESP_LOGD(TAG, "led on");
}

/**
 * @brief  Turn the LED off
 */
void led_off(void)
{
    gpio_set_level(s_gpio_num, 0);
    s_state = 0;
    ESP_LOGD(TAG, "led off");
}

/**
 * @brief  Toggle the LED state
 */
void led_toggle(void)
{
    /* Toggle based on the cached state to avoid reading the GPIO register */
    if (s_state) {
        led_off();
    } else {
        led_on();
    }
}

/**
 * @brief  Get the current LED state
 *
 * @return 1 = on, 0 = off
 */
int led_get_state(void)
{
    return s_state;
}
