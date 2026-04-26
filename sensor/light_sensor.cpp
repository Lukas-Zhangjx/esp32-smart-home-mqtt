/**
 * @file    light_sensor.cpp
 * @brief   Light intensity sensor — sensor::LightSensor class implementation + C wrappers
 *
 * AO → ADC1_CH6 (GPIO34): analog light intensity value
 * DO → GPIO13: digital threshold output, active low (bright = LOW)
 */

#include "light_sensor.h"
#include "sensor_state.h"
#include "esp_log.h"

static const char *TAG = "light_sensor";

/* ════════════════════════════════════════════════════════════════════════════
 * sensor::LightSensor implementation
 * ════════════════════════════════════════════════════════════════════════════ */

namespace sensor {

esp_err_t LightSensor::init(gpio_num_t digital_gpio, adc_channel_t adc_channel)
{
    m_digital_gpio = digital_gpio;
    m_adc_channel  = adc_channel;

    /* ── Digital input: pull-up, DO active low ── */
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << digital_gpio),
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    esp_err_t ret = gpio_config(&io_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "gpio_config failed: %d", ret);
        return ESP_FAIL;
    }

    /* ── ADC1 oneshot unit ── */
    adc_oneshot_unit_init_cfg_t unit_cfg = {
        .unit_id  = ADC_UNIT_1,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    ret = adc_oneshot_new_unit(&unit_cfg, &m_adc_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "adc_oneshot_new_unit failed: %d", ret);
        return ESP_FAIL;
    }

    /* ── Channel config: 12-bit, 12 dB attenuation (0–3.9 V range) ── */
    adc_oneshot_chan_cfg_t chan_cfg = {
        .atten    = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_12,
    };
    ret = adc_oneshot_config_channel(m_adc_handle, adc_channel, &chan_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "adc_oneshot_config_chan failed: %d", ret);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "light sensor init ok, digital=GPIO%d, adc_ch=%d",
             digital_gpio, adc_channel);
    return ESP_OK;
}

int LightSensor::digital() const
{
    /* DO is active low: GPIO=0 → bright */
    return gpio_get_level(m_digital_gpio) == 0 ? 1 : 0;
}

int LightSensor::analog() const
{
    int raw = 0;
    esp_err_t ret = adc_oneshot_read(m_adc_handle, m_adc_channel, &raw);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "adc read failed: %d", ret);
        return -1;
    }
    return raw;
}

/* static */ int LightSensor::to_percent(int raw)
{
    if (raw < 0) return 0;
    /* Lower ADC value = brighter; invert to map to 0–100 % */
    int percent = 100 - (raw * 100 / 4095);
    if (percent < 0)   percent = 0;
    if (percent > 100) percent = 100;
    return percent;
}

void LightSensor::run()
{
    int raw     = analog();
    int percent = to_percent(raw);
    int bright  = digital();
    ESP_LOGI(TAG, "lux=%d bright=%d", percent, bright);
    sensor_state_set_lux(percent);
}

} /* namespace sensor */

/* ════════════════════════════════════════════════════════════════════════════
 * C wrapper — file-scoped singleton
 * ════════════════════════════════════════════════════════════════════════════ */

static sensor::LightSensor s_light;

esp_err_t light_sensor_init(gpio_num_t digital_gpio, adc_channel_t adc_channel)
{
    return s_light.init(digital_gpio, adc_channel);
}

int  light_sensor_digital(void)        { return s_light.digital(); }
int  light_sensor_analog(void)         { return s_light.analog(); }
int  light_sensor_to_percent(int raw)  { return sensor::LightSensor::to_percent(raw); }
void light_sensor_run(void)            { s_light.run(); }
