/**
 * @file    obstacle.h
 * @brief   Obstacle / proximity sensor — C++ class + C wrapper interface
 *
 * Open-drain digital output, active low:
 *   Obstacle detected → OUT pulled LOW  (0) → reported as 1
 *   No obstacle       → OUT released, pull-up → HIGH (1) → reported as 0
 */

#ifndef OBSTACLE_H
#define OBSTACLE_H

#include "esp_err.h"
#include "driver/gpio.h"

/* ── C++ class definition ──────────────────────────────────────────────────── */
#ifdef __cplusplus

namespace sensor {

class ObstacleSensor {
public:
    ObstacleSensor() : m_gpio(GPIO_NUM_NC), m_last(-1) {}

    /**
     * @brief  Configure the GPIO as pull-up input.
     * @return ESP_OK on success, ESP_FAIL on failure.
     */
    esp_err_t init(gpio_num_t gpio_num);

    /**
     * @brief  Read the current detection state.
     * @return 1 = obstacle detected (OUT=LOW), 0 = no obstacle.
     */
    int detected() const;

    /**
     * @brief  Periodic run function — call from the 100 ms task.
     *         Reads the sensor; on state change logs and updates sensor_state door.
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

esp_err_t obstacle_init(gpio_num_t gpio_num);
int       obstacle_detected(void);
void      obstacle_run(void);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* OBSTACLE_H */
