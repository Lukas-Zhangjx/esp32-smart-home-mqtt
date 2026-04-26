/**
 * @file    relay.cpp
 * @brief   Relay / GPIO output control — gpio::Relay class implementation + C wrappers
 *
 * Push-pull output mode, active high.
 */

#include "relay.h"
#include "esp_log.h"

static const char *TAG = "relay";

/* ════════════════════════════════════════════════════════════════════════════
 * gpio::Relay implementation
 * ════════════════════════════════════════════════════════════════════════════ */

namespace gpio {

esp_err_t Relay::init(gpio_num_t gpio_num)
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
    ESP_LOGI(TAG, "relay init ok, gpio=%d", gpio_num);
    return ESP_OK;
}

int Relay::set(int on)
{
    m_state = (on != 0) ? 1 : 0;
    gpio_set_level(m_gpio, m_state);
    ESP_LOGI(TAG, "relay -> %s", m_state ? "ON" : "OFF");
    return m_state;
}

} /* namespace gpio */

/* ════════════════════════════════════════════════════════════════════════════
 * C wrapper — file-scoped singleton
 * ════════════════════════════════════════════════════════════════════════════ */

static gpio::Relay s_relay;

esp_err_t relay_init(gpio_num_t gpio_num) { return s_relay.init(gpio_num); }
int       relay_set(int state)            { return s_relay.set(state); }
int       relay_get_state(void)           { return s_relay.state(); }
