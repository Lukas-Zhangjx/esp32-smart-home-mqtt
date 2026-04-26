/**
 * @file    light_ctrl.cpp
 * @brief   Automatic light control logic — logic::LightCtrl class implementation + C wrappers
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

/* ════════════════════════════════════════════════════════════════════════════
 * logic::LightCtrl implementation
 * ════════════════════════════════════════════════════════════════════════════ */

namespace logic {

void LightCtrl::init()
{
    m_manual_on      = 0;
    m_manual_off     = 0;
    m_auto_active    = 0;
    m_last_motion_us = 0;
    relay_set(0);
    ESP_LOGI(TAG, "light ctrl init ok");
}

void LightCtrl::on_motion()
{
    /* While the user has manually turned off the light, suppress auto-trigger
     * until PIR goes idle */
    if (m_manual_off) return;

    m_last_motion_us = esp_timer_get_time();

    if (!m_auto_active) {
        m_auto_active = 1;
        relay_set(1);
        ESP_LOGI(TAG, "motion detected, light ON (auto 10s)");
    } else {
        /* Already on — just reset the timer */
        ESP_LOGD(TAG, "motion: timer reset");
    }
}

void LightCtrl::on_idle()
{
    if (m_manual_off) {
        m_manual_off = 0;
        ESP_LOGI(TAG, "pir idle, manual-off suppression cleared");
    }
}

void LightCtrl::set_manual(int on)
{
    m_manual_on = on;

    if (on) {
        /* Manual on: always on; clear suppression and auto timer */
        m_manual_off  = 0;
        m_auto_active = 0;
        relay_set(1);
        ESP_LOGI(TAG, "manual ON");
    } else {
        /* Manual off: turn off immediately; suppress auto-on until PIR goes idle */
        m_manual_off  = 1;
        m_auto_active = 0;
        relay_set(0);
        ESP_LOGI(TAG, "manual OFF");
    }
}

void LightCtrl::run()
{
    /* Nothing to do if manually always on or auto mode is not running */
    if (m_manual_on || !m_auto_active) return;

    int64_t elapsed = esp_timer_get_time() - m_last_motion_us;
    if (elapsed >= AUTO_TIMEOUT_US) {
        m_auto_active = 0;
        relay_set(0);
        ESP_LOGI(TAG, "auto timeout, light OFF");
    }
}

int LightCtrl::get_state() const
{
    return relay_get_state();
}

} /* namespace logic */

/* ════════════════════════════════════════════════════════════════════════════
 * C wrapper — file-scoped singleton
 * ════════════════════════════════════════════════════════════════════════════ */

static logic::LightCtrl s_ctrl;

void light_ctrl_init(void)           { s_ctrl.init(); }
void light_ctrl_on_motion(void)      { s_ctrl.on_motion(); }
void light_ctrl_on_idle(void)        { s_ctrl.on_idle(); }
void light_ctrl_set_manual(int on)   { s_ctrl.set_manual(on); }
void light_ctrl_run(void)            { s_ctrl.run(); }
int  light_ctrl_get_state(void)      { return s_ctrl.get_state(); }
