/**
 * @file    obstacle.h
 * @brief   Obstacle detection module public interface
 *
 * Module digital output levels:
 *   Obstacle detected → OUT = LOW  (0)
 *   No obstacle       → OUT = HIGH (1)
 */

#ifndef OBSTACLE_H
#define OBSTACLE_H

#include "esp_err.h"
#include "driver/gpio.h"

/**
 * @brief  Initialize the obstacle sensor; configure GPIO as pull-up input
 *
 * @param gpio_num  GPIO number connected to the OUT pin
 * @return ESP_OK / ESP_FAIL
 */
esp_err_t obstacle_init(gpio_num_t gpio_num);

/**
 * @brief  Read the current detection state
 *
 * @return 1 = obstacle detected, 0 = no obstacle
 */
int obstacle_detected(void);

#endif /* OBSTACLE_H */
