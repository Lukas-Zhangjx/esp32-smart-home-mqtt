/**
 * @file    dht11.h
 * @brief   DHT11 temperature and humidity sensor driver public interface
 *
 * DHT11 uses a single-wire protocol; one communication transfers 40 bits of data:
 *   8-bit humidity integer + 8-bit humidity fraction + 8-bit temperature integer
 *   + 8-bit temperature fraction + 8-bit checksum
 * The fractional parts are always 0 for DHT11.
 * Accuracy: temperature ±2°C, humidity ±5% RH
 *
 * Dependencies: esp_timer (microsecond-level delay), driver/gpio
 */

#ifndef DHT11_H
#define DHT11_H

#include "esp_err.h"
#include "driver/gpio.h"

/**
 * @brief DHT11 read result structure
 */
typedef struct {
    float temperature; /* temperature (°C) */
    float humidity;    /* relative humidity (%RH) */
} dht11_data_t;

/**
 * @brief  Initialize DHT11; configure the GPIO
 *
 * @param gpio_num  GPIO number connected to the DHT11 DATA pin
 * @return ESP_OK on success, ESP_FAIL on failure
 */
esp_err_t dht11_init(gpio_num_t gpio_num);

/**
 * @brief  Read one set of data from DHT11
 *
 * A single read takes approximately 4 ms; must not be called from an interrupt.
 * The recommended interval between reads is >= 2 seconds (DHT11 sampling period limit).
 *
 * @param data  Output: temperature and humidity data
 * @return ESP_OK               read succeeded
 * @return ESP_ERR_TIMEOUT      bus timeout (wiring issue or sensor not responding)
 * @return ESP_ERR_INVALID_CRC  checksum error (data corrupted)
 */
esp_err_t dht11_read(dht11_data_t *data);

#endif /* DHT11_H */
