/**
 * @file    led.h
 * @brief   LED GPIO control — C++ class + C wrapper interface
 *
 * Drives the LED using push-pull output mode; high level = on.
 * Wiring: GPIO → 220 Ω current-limiting resistor → LED anode → LED cathode → GND
 */

#ifndef LED_H
#define LED_H

#include "esp_err.h"
#include "driver/gpio.h"

/* ── C++ class definition ──────────────────────────────────────────────────── */
#ifdef __cplusplus

namespace gpio {

class Led {
public:
    Led() : m_gpio(GPIO_NUM_NC), m_state(0) {}

    /**
     * @brief  Configure the GPIO as push-pull output; LED starts off.
     * @return ESP_OK on success, ESP_FAIL on failure.
     */
    esp_err_t init(gpio_num_t gpio_num);

    /** @brief  Turn the LED on. */
    void on();

    /** @brief  Turn the LED off. */
    void off();

    /** @brief  Toggle the LED state. */
    void toggle();

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

esp_err_t led_init(gpio_num_t gpio_num);
void      led_on(void);
void      led_off(void);
void      led_toggle(void);
int       led_get_state(void);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* LED_H */
