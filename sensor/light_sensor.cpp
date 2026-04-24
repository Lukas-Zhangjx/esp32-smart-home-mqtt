/**
 * @file    light_sensor.c
 * @brief   Light sensor module implementation
 *
 * AO → ADC1_CH6 (GPIO34): reads the analog light intensity value
 * DO → GPIO13: digital threshold output, active low (bright = LOW)
 */

#include "light_sensor.h"
#include "esp_log.h"

static const char *TAG = "light_sensor";

/* GPIO and ADC handles */
static gpio_num_t        s_digital_gpio = GPIO_NUM_NC;
static adc_oneshot_unit_handle_t s_adc_handle = NULL;
static adc_channel_t     s_adc_channel  = ADC_CHANNEL_6;


/**
 * @brief  Initialize the light sensor
 */
esp_err_t light_sensor_init(gpio_num_t digital_gpio, adc_channel_t adc_channel)
{
    s_digital_gpio = digital_gpio;
    s_adc_channel  = adc_channel;

    /* ── Digital input: pull-up input, DO active low ── */
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

    /* ── ADC1 oneshot initialization ── */
    adc_oneshot_unit_init_cfg_t unit_cfg = {
        .unit_id  = ADC_UNIT_1,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    ret = adc_oneshot_new_unit(&unit_cfg, &s_adc_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "adc_oneshot_new_unit failed: %d", ret);
        return ESP_FAIL;
    }

    /* ── Channel configuration: 12-bit, 11 dB attenuation (range 0~3.9 V) ── */
    adc_oneshot_chan_cfg_t chan_cfg = {
        .bitwidth = ADC_BITWIDTH_12,
        .atten    = ADC_ATTEN_DB_11,
    };
    ret = adc_oneshot_config_channel(s_adc_handle, adc_channel, &chan_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "adc_oneshot_config_chan failed: %d", ret);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "light sensor init ok, digital=GPIO%d, adc_ch=%d", digital_gpio, adc_channel);
    return ESP_OK;
}


/**
 * @brief  Read the digital output state
 *
 * @return 1 = bright, 0 = dark
 */
int light_sensor_digital(void)
{
    /* DO is active low: GPIO=0 → bright */
    return gpio_get_level(s_digital_gpio) == 0 ? 1 : 0;
}


/**
 * @brief  Read the raw ADC value
 *
 * @return 0-4095, returns -1 on failure
 */
int light_sensor_analog(void)
{
    int raw = 0;
    esp_err_t ret = adc_oneshot_read(s_adc_handle, s_adc_channel, &raw);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "adc read failed: %d", ret);
        return -1;
    }
    return raw;
}


/**
 * @brief  Convert a raw ADC value to a light intensity percentage (0=dark, 100=brightest)
 */
int light_sensor_to_percent(int raw)
{
    if (raw < 0) return 0;
    /* Higher light → lower LDR resistance → lower voltage divider → lower ADC value; invert to map */
    int percent = 100 - (raw * 100 / 4095);
    if (percent < 0)   percent = 0;
    if (percent > 100) percent = 100;
    return percent;
}
