/**
 * @file    ir_sensor.cpp
 * @brief   HC-SR501 PIR motion sensor — sensor::IrSensor class implementation + C wrappers
 *
 * Active high: OUT=HIGH when motion is detected.
 * HC-SR501 drives the output actively so no internal pull-up is needed.
 */

#include "ir_sensor.h"
#include "esp_log.h"

static const char *TAG = "ir_sensor";

/* ════════════════════════════════════════════════════════════════════════════
 * sensor::IrSensor implementation
 * ════════════════════════════════════════════════════════════════════════════ */

namespace sensor {

esp_err_t IrSensor::init(gpio_num_t gpio_num)
{
    m_gpio = gpio_num;

    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << gpio_num),
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,   /* HC-SR501 is self-driven */
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

int IrSensor::detected() const
{
    /* Active high: HIGH → motion detected */
    return gpio_get_level(m_gpio);
}

} /* namespace sensor */

/* ════════════════════════════════════════════════════════════════════════════
 * C wrapper — file-scoped singleton
 * ════════════════════════════════════════════════════════════════════════════ */

static sensor::IrSensor s_ir;

esp_err_t ir_sensor_init(gpio_num_t gpio_num) { return s_ir.init(gpio_num); }
int       ir_sensor_detected(void)            { return s_ir.detected(); }
