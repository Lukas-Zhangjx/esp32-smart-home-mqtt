/**
 * @file    light_sensor.h
 * @brief   Light intensity sensor — C++ class + C wrapper interface
 *
 * Supports two output channels:
 *   AO (analog)  → ADC1_CH6 (GPIO34), raw value 0–4095 (smaller = brighter)
 *   DO (digital) → GPIO13, active low (LOW = bright, exceeds threshold)
 *
 * ADC uses the ESP-IDF v5.x adc_oneshot driver.
 */

#ifndef LIGHT_SENSOR_H
#define LIGHT_SENSOR_H

#include "esp_err.h"
#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"

/* ── C++ class definition ──────────────────────────────────────────────────── */
#ifdef __cplusplus

namespace sensor {

class LightSensor {
public:
    LightSensor() : m_digital_gpio(GPIO_NUM_NC),
                    m_adc_handle(nullptr),
                    m_adc_channel(ADC_CHANNEL_6) {}

    /**
     * @brief  Initialize the digital GPIO input and the ADC oneshot unit.
     * @param  digital_gpio  DO pin (e.g. GPIO_NUM_13).
     * @param  adc_channel   ADC1 channel for AO (e.g. ADC_CHANNEL_6 = GPIO34).
     * @return ESP_OK on success, ESP_FAIL on failure.
     */
    esp_err_t init(gpio_num_t digital_gpio, adc_channel_t adc_channel);

    /**
     * @brief  Read the digital threshold output.
     * @return 1 = bright (DO=LOW, light exceeds threshold), 0 = dark.
     */
    int digital() const;

    /**
     * @brief  Read the raw ADC value.
     * @return 0–4095; -1 on read failure.
     */
    int analog() const;

    /**
     * @brief  Convert a raw ADC value to a brightness percentage.
     * @param  raw  Raw ADC value 0–4095.
     * @return 0–100; higher = brighter.
     */
    static int to_percent(int raw);

    /**
     * @brief  Periodic run function — call from the 2000 ms task.
     *         Reads analog and digital outputs; updates sensor_state lux.
     */
    void run();

private:
    gpio_num_t               m_digital_gpio;
    adc_oneshot_unit_handle_t m_adc_handle;
    adc_channel_t            m_adc_channel;
};

} /* namespace sensor */

extern "C" {
#endif /* __cplusplus */

/* ── C interface (callable from main.c) ────────────────────────────────────── */

esp_err_t light_sensor_init(gpio_num_t digital_gpio, adc_channel_t adc_channel);
int       light_sensor_digital(void);
int       light_sensor_analog(void);
int       light_sensor_to_percent(int raw);
void      light_sensor_run(void);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* LIGHT_SENSOR_H */
