/**
 * @file    relay.h
 * @brief   Relay / GPIO output control module public interface
 *
 * Push-pull output mode, active high:
 *   state = 1 → GPIO high → load conducting
 *   state = 0 → GPIO low  → load open
 */

#ifndef RELAY_H
#define RELAY_H

#include "esp_err.h"
#include "driver/gpio.h"

/**
 * @brief  Initialize the relay GPIO; configure as push-pull output, off by default
 *
 * @param gpio_num  GPIO number connected to the relay/LED
 * @return ESP_OK / ESP_FAIL
 */
esp_err_t relay_init(gpio_num_t gpio_num);

/**
 * @brief  Set the relay state
 *
 * @param state  1 = on (conducting), 0 = off (open)
 * @return       The actual state after being set
 */
int relay_set(int state);

/**
 * @brief  Get the current relay state
 *
 * @return 1 = on (conducting), 0 = off (open)
 */
int relay_get_state(void);

#endif /* RELAY_H */
