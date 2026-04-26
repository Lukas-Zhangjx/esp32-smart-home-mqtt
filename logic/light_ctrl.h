/**
 * @file    light_ctrl.h
 * @brief   Automatic light control logic — C++ class + C wrapper interface
 *
 * Control rules:
 *   1. Motion detected → auto light on; auto off 10 seconds after the last motion
 *   2. Motion detected again → reset the 10-second timer
 *   3. Manual (MQTT) light on → always on, unaffected by auto timer
 *   4. Manual (MQTT) light off → turn off immediately, suppress auto-on until PIR goes idle
 *
 * Dependencies: relay module (gpio/relay.h), esp_timer
 */

#ifndef LIGHT_CTRL_H
#define LIGHT_CTRL_H

/* ── C++ class definition ──────────────────────────────────────────────────── */
#ifdef __cplusplus
#include <cstdint>

namespace logic {

class LightCtrl {
public:
    LightCtrl() : m_manual_on(0), m_manual_off(0),
                  m_auto_active(0), m_last_motion_us(0) {}

    /** @brief  Initialize: reset state and turn relay off. */
    void init();

    /** @brief  Call when PIR detects motion. Turns light on and (re)starts timer. */
    void on_motion();

    /** @brief  Call when PIR goes idle (no motion). Clears manual-off suppression. */
    void on_idle();

    /**
     * @brief  Manual control from MQTT.
     * @param  on  1 = force on, 0 = force off.
     */
    void set_manual(int on);

    /**
     * @brief  Periodic run function — call from the 100 ms task.
     *         Checks the auto-off countdown timer.
     */
    void run();

    /** @brief  Return actual relay state: 1 = on, 0 = off. */
    int get_state() const;

private:
    int     m_manual_on;       /* 1 = manually always on */
    int     m_manual_off;      /* 1 = suppress auto-on until PIR goes idle */
    int     m_auto_active;     /* 1 = auto mode running, counting down */
    int64_t m_last_motion_us;  /* timestamp of last motion (esp_timer_get_time) */
};

} /* namespace logic */

extern "C" {
#endif /* __cplusplus */

/* ── C interface (callable from main.c) ────────────────────────────────────── */

void light_ctrl_init(void);
void light_ctrl_on_motion(void);
void light_ctrl_on_idle(void);
void light_ctrl_set_manual(int on);
void light_ctrl_run(void);
int  light_ctrl_get_state(void);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* LIGHT_CTRL_H */
