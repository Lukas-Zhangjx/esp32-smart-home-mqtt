/**
 * @file    light_ctrl.h
 * @brief   Automatic light control logic module
 *
 * Control rules:
 *   1. Motion detected → auto light on; auto off 30 seconds after the last motion
 *   2. Motion detected again → reset the 30-second timer
 *   3. Manual (web) light on → always on, unaffected by auto timer
 *   4. Manual (web) light off → turn off immediately, clear auto timer
 *
 * Dependencies: relay module (gpio/relay.h), esp_timer
 */

#ifndef LIGHT_CTRL_H
#define LIGHT_CTRL_H

/**
 * @brief  Initialize the light control module; light off by default
 */
void light_ctrl_init(void);

/**
 * @brief  Notify the control module that human motion has been detected
 *
 * Turns on the light automatically and resets the 30-second countdown timer.
 * Should be called from io_task when HC-SR501 triggers.
 */
void light_ctrl_on_motion(void);

/**
 * @brief  Manually set the light state (from the web switch)
 *
 * @param on  1 = manual on, 0 = manual off
 */
void light_ctrl_set_manual(int on);

/**
 * @brief  Notify the control module that PIR has gone idle (no person present)
 *
 * Clears the manual-off suppression so that the next person entering can
 * trigger auto-on again.
 * Should be called from io_task when ir=0 is detected.
 */
void light_ctrl_on_idle(void);

/**
 * @brief  Periodically check the auto-off countdown timer; should be called every io_task loop
 */
void light_ctrl_tick(void);

/**
 * @brief  Get the current actual light state
 *
 * @return 1 = on, 0 = off
 */
int light_ctrl_get_state(void);

#endif /* LIGHT_CTRL_H */
