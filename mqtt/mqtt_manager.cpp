/**
 * @file    mqtt_manager.cpp
 * @brief   MQTT client manager implementation
 *
 * Wraps the ESP-IDF esp_mqtt_client in a C++ class (mqtt::Manager).
 * A file-scoped singleton pointer is maintained for the C wrapper functions.
 *
 * Event flow:
 *   ESP-IDF MQTT task  →  s_event_cb (static)  →  on_event (instance)
 *
 * Dependencies: esp_mqtt, light_ctrl
 */

#include "mqtt_manager.h"
#include "light_ctrl.h"
#include "esp_log.h"
#include <cstdio>
#include <cstring>

static const char *TAG = "mqtt_manager";

/* ════════════════════════════════════════════════════════════════════════════
 * mqtt::Manager implementation
 * ════════════════════════════════════════════════════════════════════════════ */

namespace mqtt {

/* ── Constructor ─────────────────────────────────────────────────────────── */

Manager::Manager(const char *broker_uri)
    : m_handle(nullptr), m_state(ConnectState::IDLE)
{
    /* Zero-init the config struct first, then set only what we need.
     * Required in C++ because designated initialisers cannot be nested. */
    esp_mqtt_client_config_t cfg = {};
    cfg.broker.address.uri = broker_uri;

    m_handle = esp_mqtt_client_init(&cfg);
    if (m_handle == nullptr) {
        ESP_LOGE(TAG, "esp_mqtt_client_init failed");
        m_state = ConnectState::ERROR;
    }
}

/* ── Destructor ──────────────────────────────────────────────────────────── */

Manager::~Manager()
{
    if (m_handle) {
        esp_mqtt_client_stop(m_handle);
        esp_mqtt_client_destroy(m_handle);
        m_handle = nullptr;
    }
}

/* ── start ───────────────────────────────────────────────────────────────── */

esp_err_t Manager::start()
{
    if (m_handle == nullptr) {
        return ESP_FAIL;  /* constructor already logged the error */
    }

    /* Pass `this` as the arg so the static callback can reach the instance */
    esp_err_t ret = esp_mqtt_client_register_event(
        m_handle, MQTT_EVENT_ANY, s_event_cb, this);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "register_event failed: %d", ret);
        return ret;
    }

    ret = esp_mqtt_client_start(m_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "client_start failed: %d", ret);
        return ret;
    }

    m_state = ConnectState::CONNECTING;
    ESP_LOGI(TAG, "MQTT client started, connecting to broker...");
    return ESP_OK;
}

/* ── publish ─────────────────────────────────────────────────────────────── */

int Manager::publish(const char *topic, const char *payload,
                     int qos, bool retain)
{
    if (!is_connected()) {
        return -1;  /* drop silently when offline — caller should not block */
    }

    /* Pass 0 for length so the library uses strlen(payload) */
    int msg_id = esp_mqtt_client_publish(
        m_handle, topic, payload, 0, qos, retain ? 1 : 0);

    if (msg_id < 0) {
        ESP_LOGW(TAG, "publish failed on topic: %s", topic);
    }
    return msg_id;
}

/* ── publish_sensors ─────────────────────────────────────────────────────── */

void Manager::publish_sensors(float temp, float hum,
                               int motion, int door,
                               int lux, int relay_state)
{
    if (!is_connected()) {
        return;
    }

    char buf[16];

    snprintf(buf, sizeof(buf), "%.1f", temp);
    publish(topics::TEMPERATURE, buf);

    snprintf(buf, sizeof(buf), "%.1f", hum);
    publish(topics::HUMIDITY, buf);

    snprintf(buf, sizeof(buf), "%d", motion);
    publish(topics::MOTION, buf);

    snprintf(buf, sizeof(buf), "%d", door);
    publish(topics::DOOR, buf);

    snprintf(buf, sizeof(buf), "%d", lux);
    publish(topics::LUX, buf);

    snprintf(buf, sizeof(buf), "%d", relay_state);
    publish(topics::RELAY_STATE, buf);
}

/* ── Static event callback ───────────────────────────────────────────────── */

void Manager::s_event_cb(void *arg, esp_event_base_t /*base*/,
                          int32_t /*event_id*/, void *data)
{
    /* Recover the Manager instance from the arg set in register_event() */
    auto *self = static_cast<Manager *>(arg);
    self->on_event(static_cast<esp_mqtt_event_handle_t>(data));
}

/* ── Instance event handler ──────────────────────────────────────────────── */

void Manager::on_event(esp_mqtt_event_handle_t event)
{
    switch (event->event_id) {

    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "connected to broker");
        m_state = ConnectState::CONNECTED;

        /* Re-subscribe on every (re)connect so commands work after a dropout */
        esp_mqtt_client_subscribe(m_handle, topics::RELAY_SET, 1);
        ESP_LOGI(TAG, "subscribed to %s", topics::RELAY_SET);
        break;

    case MQTT_EVENT_DISCONNECTED:
        /* ESP-IDF MQTT client retries automatically; no action needed here */
        ESP_LOGW(TAG, "disconnected — retrying automatically");
        m_state = ConnectState::DISCONNECTED;
        break;

    case MQTT_EVENT_DATA:
        /* Incoming message — check topic and act */
        if (event->topic_len > 0) {
            /* strncmp is safe because topic is not null-terminated in the event */
            bool is_relay_cmd = (strncmp(event->topic,
                                         topics::RELAY_SET,
                                         event->topic_len) == 0);
            if (is_relay_cmd && event->data_len > 0) {
                bool on = (event->data[0] == '1');
                ESP_LOGI(TAG, "relay command via MQTT: %s", on ? "ON" : "OFF");
                light_ctrl_set_manual(on ? 1 : 0);
            }
        }
        break;

    case MQTT_EVENT_ERROR:
        ESP_LOGE(TAG, "MQTT error type: %d",
                 event->error_handle->error_type);
        m_state = ConnectState::ERROR;
        break;

    default:
        break;
    }
}

} /* namespace mqtt */

/* ════════════════════════════════════════════════════════════════════════════
 * C wrapper — file-scoped singleton
 * ════════════════════════════════════════════════════════════════════════════ */

/* Singleton pointer; allocated once in mqtt_manager_init, never freed
 * (lives for the duration of the firmware run). */
static mqtt::Manager *s_manager = nullptr;

esp_err_t mqtt_manager_init(const char *broker_uri)
{
    const char *uri = (broker_uri != nullptr) ? broker_uri : MQTT_BROKER_URI;
    ESP_LOGI(TAG, "initializing MQTT manager, broker: %s", uri);

    s_manager = new mqtt::Manager(uri);
    return s_manager->start();
}

void mqtt_manager_publish_sensors(float temp, float hum,
                                  int motion, int door,
                                  int lux, int relay_state)
{
    if (s_manager) {
        s_manager->publish_sensors(temp, hum, motion, door, lux, relay_state);
    }
}

bool mqtt_manager_is_connected(void)
{
    return s_manager && s_manager->is_connected();
}
