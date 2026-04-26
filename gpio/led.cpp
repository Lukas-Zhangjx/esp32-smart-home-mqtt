/**
 * @file    led.cpp
 * @brief   LED GPIO control — gpio::Led class implementation + C wrappers
 *
 * Push-pull output mode: GPIO high = LED on, GPIO low = LED off.
 * The current state is cached in m_state to avoid reading the GPIO register.
 */

#include "led.h"
#include "esp_log.h"

static const char *TAG = "led";

/* ════════════════════════════════════════════════════════════════════════════
 * gpio::Led implementation
 * ════════════════════════════════════════════════════════════════════════════ */

namespace gpio {

esp_err_t Led::init(gpio_num_t gpio_num)
{
    m_gpio  = gpio_num;
    m_state = 0;

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

    gpio_set_level(m_gpio, 0);
    ESP_LOGI(TAG, "led init ok, gpio=%d", gpio_num);
    return ESP_OK;
}

void Led::on()
{
    gpio_set_level(m_gpio, 1);
    m_state = 1;
    ESP_LOGD(TAG, "led on");
}

void Led::off()
{
    gpio_set_level(m_gpio, 0);
    m_state = 0;
    ESP_LOGD(TAG, "led off");
}

void Led::toggle()
{
    if (m_state) { off(); } else { on(); }
}

} /* namespace gpio */

/* ════════════════════════════════════════════════════════════════════════════
 * C wrapper — file-scoped singleton
 * ════════════════════════════════════════════════════════════════════════════ */

static gpio::Led s_led;

esp_err_t led_init(gpio_num_t gpio_num) { return s_led.init(gpio_num); }
void      led_on(void)                  { s_led.on(); }
void      led_off(void)                 { s_led.off(); }
void      led_toggle(void)              { s_led.toggle(); }
int       led_get_state(void)           { return s_led.state(); }
