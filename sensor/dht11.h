/**
 * @file    dht11.h
 * @brief   DHT11 temperature and humidity sensor — C++ class + C wrapper interface
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
 * @brief DHT11 read result structure (shared between C and C++ code)
 */
typedef struct {
    float temperature; /* temperature (°C) */
    float humidity;    /* relative humidity (%RH) */
} dht11_data_t;

/* ── C++ class definition ──────────────────────────────────────────────────── */
#ifdef __cplusplus

namespace sensor {

class DhtSensor {
public:
    DhtSensor() : m_gpio(GPIO_NUM_NC) {}

    /**
     * @brief  Configure the GPIO as open-drain I/O; waits 1 s for DHT11 power-on stabilisation.
     * @return ESP_OK on success, ESP_FAIL on failure.
     */
    esp_err_t init(gpio_num_t gpio_num);

    /**
     * @brief  Read one set of data from the DHT11 (~4 ms blocking).
     *         Must not be called from an interrupt.
     *         Recommended interval between calls: >= 2 seconds.
     * @param  data  Output: temperature and humidity.
     * @return ESP_OK / ESP_ERR_TIMEOUT / ESP_ERR_INVALID_CRC
     */
    esp_err_t read(dht11_data_t *data);

private:
    gpio_num_t m_gpio;

    /**
     * @brief  Spin-wait until the DATA line reaches @p level.
     * @return Elapsed time in μs, or -1 on timeout (> 200 μs).
     */
    int wait_for_level(int level) const;
};

} /* namespace sensor */

extern "C" {
#endif /* __cplusplus */

/* ── C interface (callable from main.c) ────────────────────────────────────── */

esp_err_t dht11_init(gpio_num_t gpio_num);
esp_err_t dht11_read(dht11_data_t *data);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* DHT11_H */
