/**
 * @file    ir_sensor.h
 * @brief   IR detection module public interface (digital output type)
 *
 * Suitable for IR sensor modules with digital output (e.g. FC-51, TCRT5000, etc.).
 * Module output levels:
 *   Target detected (blocked/reflected) → OUT = LOW  (0)
 *   No target                           → OUT = HIGH (1)
 */

#ifndef IR_SENSOR_H
#define IR_SENSOR_H

#include "esp_err.h"
#include "driver/gpio.h"

/**
 * @brief  Initialize the IR sensor; configure GPIO as pull-up input
 *
 * @param gpio_num  GPIO number connected to the OUT pin
 * @return ESP_OK / ESP_FAIL
 */
esp_err_t ir_sensor_init(gpio_num_t gpio_num);

/**
 * @brief  Read the current detection state
 *
 * @return 1 = target detected, 0 = no target
 */
int ir_sensor_detected(void);

#endif /* IR_SENSOR_H */
