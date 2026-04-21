/**
 * @file    led.h
 * @brief   LED GPIO control module public interface
 *
 * Drives the LED using push-pull output mode; high level = on.
 * Wiring: GPIO → 220 Ω current-limiting resistor → LED anode → LED cathode → GND
 */

#ifndef LED_H
#define LED_H

#include "esp_err.h"
#include "driver/gpio.h"

/**
 * @brief  Initialize the LED GPIO; configure as push-pull output, off by default
 *
 * @param gpio_num  GPIO number connected to the LED
 * @return ESP_OK on success, ESP_FAIL on failure
 */
esp_err_t led_init(gpio_num_t gpio_num);

/**
 * @brief  Turn the LED on
 */
void led_on(void);

/**
 * @brief  Turn the LED off
 */
void led_off(void);

/**
 * @brief  Toggle the LED state
 */
void led_toggle(void);

/**
 * @brief  Get the current LED state
 *
 * @return 1 = on, 0 = off
 */
int led_get_state(void);

#endif /* LED_H */
