/**
 * @file    obstacle.cpp
 * @brief   Obstacle / proximity sensor — sensor::ObstacleSensor class implementation + C wrappers
 *
 * Open-drain active-low output:
 *   OUT pulled low  → GPIO reads 0 → obstacle present  → returned as 1
 *   OUT released    → pull-up HIGH → GPIO reads 1 → no obstacle → returned as 0
 */

#include "obstacle.h"
#include "sensor_state.h"
#include "esp_log.h"

static const char *TAG = "obstacle";

/* ════════════════════════════════════════════════════════════════════════════
 * sensor::ObstacleSensor implementation
 * ════════════════════════════════════════════════════════════════════════════ */

namespace sensor {

esp_err_t ObstacleSensor::init(gpio_num_t gpio_num)
{
    m_gpio = gpio_num;

    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << gpio_num),
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_ENABLE,    /* keeps line HIGH when sensor releases */
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

int ObstacleSensor::detected() const
{
    /* Active low: GPIO=0 means obstacle present; invert for logical result */
    return gpio_get_level(m_gpio) == 0 ? 1 : 0;
}

void ObstacleSensor::run()
{
    int current = detected();
    if (current != m_last) {
        ESP_LOGI(TAG, "door: %s", current ? "CLOSED" : "OPEN");
        sensor_state_set_door(current);
        m_last = current;
    }
}

} /* namespace sensor */

/* ════════════════════════════════════════════════════════════════════════════
 * C wrapper — file-scoped singleton
 * ════════════════════════════════════════════════════════════════════════════ */

static sensor::ObstacleSensor s_obstacle;

esp_err_t obstacle_init(gpio_num_t gpio_num) { return s_obstacle.init(gpio_num); }
int       obstacle_detected(void)            { return s_obstacle.detected(); }
void      obstacle_run(void)                 { s_obstacle.run(); }
