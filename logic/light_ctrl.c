/**
 * @file    light_ctrl.c
 * @brief   Automatic light control logic module implementation
 *
 * State machine:
 *
 *   ┌──────────────────────────────────────────┐
 *   │  manual_on=0, auto_active=0 → light OFF  │ ← initial / manual off / after timeout
 *   └──────┬───────────────────────────────────┘
 *          │ motion detected          manual on
 *          ▼                              ▼
 *   ┌─────────────────┐        ┌──────────────────┐
 *   │ auto_active=1   │        │ manual_on=1      │
 *   │ light ON, 10s   │        │ light ON, stay   │
 *   │ countdown       │        │                  │
 *   └──────┬──────────┘        └──────────────────┘
 *          │ timeout (10s no motion)    │ manual off
 *          ▼                            ▼
 *        light OFF              manual_off=1 (suppress auto)
 *                                       │ PIR goes idle
 *                                       ▼
 *                                  manual_off=0 (restored)
 */

#include "light_ctrl.h"
#include "relay.h"
#include "esp_timer.h"
#include "esp_log.h"

static const char *TAG = "light_ctrl";

/* Auto-off timeout: 10 seconds */
#define AUTO_TIMEOUT_US  (10LL * 1000 * 1000)

/* Manual on state: 1 = manually always on, 0 = not manually on */
static int s_manual_on = 0;

/* Manual off flag: set to 1 when the user explicitly turns off the light;
 * cleared when PIR goes idle; suppresses auto-on in the meantime */
static int s_manual_off = 0;

/* Whether auto mode is active */
static int s_auto_active = 0;

/* Timestamp of the last detected motion (microseconds) */
static int64_t s_last_motion_us = 0;


/**
 * @brief  Initialize the light control module; light off by default
 */
void light_ctrl_init(void)
{
    s_manual_on      = 0;
    s_manual_off     = 0;
    s_auto_active    = 0;
    s_last_motion_us = 0;
    relay_set(0);
    ESP_LOGI(TAG, "light ctrl init ok");
}


/**
 * @brief  Notify the control module that human motion has been detected
 */
void light_ctrl_on_motion(void)
{
    /* While the user has manually turned off the light, suppress auto-trigger
     * until PIR goes idle */
    if (s_manual_off) return;

    s_last_motion_us = esp_timer_get_time();

    if (!s_auto_active) {
        s_auto_active = 1;
        relay_set(1);
        ESP_LOGI(TAG, "motion detected, light ON (auto 10s)");
    } else {
        /* Already on; just reset the timer */
        ESP_LOGD(TAG, "motion: timer reset");
    }
}


/**
 * @brief  Notify the control module that PIR has gone idle (no person present)
 *
 * Used to clear the manual-off suppression so that the next person entering
 * can trigger auto-on again.
 */
void light_ctrl_on_idle(void)
{
    if (s_manual_off) {
        s_manual_off = 0;
        ESP_LOGI(TAG, "pir idle, manual-off suppression cleared");
    }
}


/**
 * @brief  Manually set the light state (from the web switch)
 */
void light_ctrl_set_manual(int on)
{
    s_manual_on = on;

    if (on) {
        /* Manual on: always on, clear all suppression and auto timer */
        s_manual_off  = 0;
        s_auto_active = 0;
        relay_set(1);
        ESP_LOGI(TAG, "manual ON");
    } else {
        /* Manual off: turn off immediately, suppress auto-on until PIR goes idle */
        s_manual_off  = 1;
        s_auto_active = 0;
        relay_set(0);
        ESP_LOGI(TAG, "manual OFF");
    }
}


/**
 * @brief  Periodically check the auto-off countdown timer
 *
 * Should be called every io_task loop iteration (100 ms).
 */
void light_ctrl_tick(void)
{
    /* No check needed when manually always on or when auto mode is not active */
    if (s_manual_on || !s_auto_active) return;

    int64_t elapsed = esp_timer_get_time() - s_last_motion_us;
    if (elapsed >= AUTO_TIMEOUT_US) {
        s_auto_active = 0;
        relay_set(0);
        ESP_LOGI(TAG, "auto timeout, light OFF");
    }
}


/**
 * @brief  Get the current actual light state
 */
int light_ctrl_get_state(void)
{
    return relay_get_state();
}
