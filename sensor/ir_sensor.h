/**
 * @file    ir_sensor.h
 * @brief   HC-SR501 PIR motion sensor — C++ class + C wrapper interface
 *
 * Digital output, active high:
 *   Motion detected → OUT = HIGH (1)
 *   No motion       → OUT = LOW  (0)
 */

#ifndef IR_SENSOR_H
#define IR_SENSOR_H

#include "esp_err.h"
#include "driver/gpio.h"

/* ── C++ class definition ──────────────────────────────────────────────────── */
#ifdef __cplusplus

namespace sensor {

class IrSensor {
public:
    IrSensor() : m_gpio(GPIO_NUM_NC), m_last(-1) {}

    /**
     * @brief  Configure the GPIO as floating input (HC-SR501 drives the line actively).
     * @return ESP_OK on success, ESP_FAIL on failure.
     */
    esp_err_t init(gpio_num_t gpio_num);

    /**
     * @brief  Read the current detection state.
     * @return 1 = motion detected (OUT=HIGH), 0 = no motion.
     */
    int detected() const;

    /**
     * @brief  Periodic run function — call from the 100 ms task.
     *         Reads the sensor; on state change logs and updates sensor_state motion.
     *         Notifies light_ctrl on every call (resets the auto-off timer while motion
     *         is continuously detected).
     */
    void run();

private:
    gpio_num_t m_gpio;
    int        m_last;  /* previous reading; -1 = not yet sampled */
};

} /* namespace sensor */

extern "C" {
#endif /* __cplusplus */

/* ── C interface (callable from main.c) ────────────────────────────────────── */

esp_err_t ir_sensor_init(gpio_num_t gpio_num);
int       ir_sensor_detected(void);
void      ir_sensor_run(void);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* IR_SENSOR_H */
