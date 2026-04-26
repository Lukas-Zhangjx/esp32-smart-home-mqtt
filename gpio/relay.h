/**
 * @file    relay.h
 * @brief   Relay / GPIO output control — C++ class + C wrapper interface
 *
 * Push-pull output mode, active high:
 *   state = 1 → GPIO high → load conducting
 *   state = 0 → GPIO low  → load open
 */

#ifndef RELAY_H
#define RELAY_H

#include "esp_err.h"
#include "driver/gpio.h"

/* ── C++ class definition ──────────────────────────────────────────────────── */
#ifdef __cplusplus

namespace gpio {

class Relay {
public:
    Relay() : m_gpio(GPIO_NUM_NC), m_state(0) {}

    /**
     * @brief  Configure the GPIO as push-pull output; relay starts off.
     * @return ESP_OK on success, ESP_FAIL on failure.
     */
    esp_err_t init(gpio_num_t gpio_num);

    /**
     * @brief  Set the relay state.
     * @param  on  1 = on (conducting), 0 = off (open).
     * @return The actual state after being set.
     */
    int set(int on);

    /** @brief  Return current state: 1 = on, 0 = off. */
    int state() const { return m_state; }

private:
    gpio_num_t m_gpio;
    int        m_state;
};

} /* namespace gpio */

extern "C" {
#endif /* __cplusplus */

/* ── C interface (callable from main.c) ────────────────────────────────────── */

esp_err_t relay_init(gpio_num_t gpio_num);
int       relay_set(int state);
int       relay_get_state(void);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* RELAY_H */
