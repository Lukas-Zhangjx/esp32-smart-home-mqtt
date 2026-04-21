/**
 * @file    light_sensor.h
 * @brief   Light sensor module public interface
 *
 * Supports two output channels:
 *   AO (analog)  → ADC1_CH6 (GPIO34), reads raw light intensity value 0-4095
 *   DO (digital) → GPIO13, low level means light exceeds threshold (bright),
 *                  high level means dark
 *
 * ADC uses the ESP-IDF v5.x adc_oneshot driver.
 */

#ifndef LIGHT_SENSOR_H
#define LIGHT_SENSOR_H

#include "esp_err.h"
#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"

/**
 * @brief  Initialize the light sensor
 *
 * @param digital_gpio  DO pin number (digital output, GPIO13)
 * @param adc_channel   ADC1 channel corresponding to AO (ADC_CHANNEL_6 for GPIO34)
 * @return ESP_OK / ESP_FAIL
 */
esp_err_t light_sensor_init(gpio_num_t digital_gpio, adc_channel_t adc_channel);

/**
 * @brief  Read the digital output state
 *
 * @return 1 = bright (DO=LOW, light exceeds threshold), 0 = dark (DO=HIGH)
 */
int light_sensor_digital(void);

/**
 * @brief  Read the analog light intensity
 *
 * @return Raw ADC value 0-4095; smaller value means brighter
 *         (LDR resistance decreases as light increases)
 */
int light_sensor_analog(void);

/**
 * @brief  Convert a raw ADC value to a light intensity percentage
 *
 * @param raw  Raw ADC value 0-4095
 * @return     0-100; higher value means brighter
 */
int light_sensor_to_percent(int raw);

#endif /* LIGHT_SENSOR_H */
