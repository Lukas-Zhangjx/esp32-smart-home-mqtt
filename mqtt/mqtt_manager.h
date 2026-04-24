/**
 * @file    mqtt_manager.h
 * @brief   MQTT client manager — publishes sensor data and subscribes to relay control
 *
 * C++ class mqtt::Manager wraps the ESP-IDF MQTT client.
 * A C interface (extern "C") is exposed so main.c can call it without C++ knowledge.
 *
 * Topics published:
 *   home/sensor/temperature  — float, e.g. "24.5"
 *   home/sensor/humidity     — float, e.g. "61.0"
 *   home/sensor/motion       — "1" or "0"
 *   home/sensor/door         — "1" (closed) or "0" (open)
 *   home/sensor/lux          — integer percentage 0-100
 *   home/relay/state         — "1" (on) or "0" (off)
 *
 * Topic subscribed:
 *   home/relay/set           — "1" to turn on, "0" to turn off
 *
 * Dependencies: esp_mqtt, light_ctrl
 */

#ifndef MQTT_MANAGER_H
#define MQTT_MANAGER_H

#include "esp_err.h"
#include <stdbool.h>

/* Default public broker — change to your own broker IP for production */
#ifndef MQTT_BROKER_URI
#define MQTT_BROKER_URI "mqtt://broker.hivemq.com"
#endif

/* ── C++ class definition ──────────────────────────────────────────────────── */
#ifdef __cplusplus

#include "mqtt_client.h"  /* ESP-IDF MQTT client handle */
#include <cstdint>

namespace mqtt {

/* ── Topic string constants ────────────────────────────────────────────────── */
namespace topics {
    constexpr const char *TEMPERATURE = "home/sensor/temperature";
    constexpr const char *HUMIDITY    = "home/sensor/humidity";
    constexpr const char *MOTION      = "home/sensor/motion";
    constexpr const char *DOOR        = "home/sensor/door";
    constexpr const char *LUX         = "home/sensor/lux";
    constexpr const char *RELAY_STATE = "home/relay/state";
    constexpr const char *RELAY_SET   = "home/relay/set";  /* subscribed */
} /* namespace topics */

/* ── Connection state ──────────────────────────────────────────────────────── */
enum class ConnectState : uint8_t {
    IDLE,
    CONNECTING,
    CONNECTED,
    DISCONNECTED,
    ERROR
};

/* ── Manager class ─────────────────────────────────────────────────────────── */
class Manager {
public:
    /**
     * @brief  Construct the MQTT manager.
     * @param  broker_uri  Broker URI, e.g. "mqtt://192.168.1.10:1883"
     */
    explicit Manager(const char *broker_uri);

    /**
     * @brief  Destructor — stops and destroys the ESP-IDF MQTT client handle.
     */
    ~Manager();

    /**
     * @brief  Register the event handler and start the MQTT client.
     *         Call this after WiFi is connected.
     * @return ESP_OK on success, ESP_FAIL if the client handle is invalid.
     */
    esp_err_t start();

    /**
     * @brief  Publish a string payload to a topic.
     * @param  topic    Null-terminated topic string.
     * @param  payload  Null-terminated payload string.
     * @param  qos      QoS level (0, 1, or 2). Default 0.
     * @param  retain   Retain flag. Default false.
     * @return Message ID >= 0 on success, -1 if not connected or on failure.
     */
    int publish(const char *topic, const char *payload,
                int qos = 0, bool retain = false);

    /**
     * @brief  Publish all sensor readings in a single call.
     * @param  temp         Temperature in °C.
     * @param  hum          Relative humidity in %.
     * @param  motion       1 = motion detected, 0 = clear.
     * @param  door         1 = door closed, 0 = door open.
     * @param  lux          Brightness percentage 0–100.
     * @param  relay_state  1 = relay on, 0 = relay off.
     */
    void publish_sensors(float temp, float hum,
                         int motion, int door,
                         int lux, int relay_state);

    /** @brief  Current connection state. */
    ConnectState state() const { return m_state; }

    /** @brief  Returns true when the client is connected to the broker. */
    bool is_connected() const { return m_state == ConnectState::CONNECTED; }

private:
    esp_mqtt_client_handle_t m_handle;  /* ESP-IDF client handle */
    ConnectState             m_state;

    /**
     * @brief  Static event callback required by the ESP-IDF C API.
     *         Forwards to the instance method via the arg pointer.
     */
    static void s_event_cb(void *arg, esp_event_base_t base,
                           int32_t event_id, void *data);

    /**
     * @brief  Instance-level event dispatcher — handles connect, disconnect, data, error.
     */
    void on_event(esp_mqtt_event_handle_t event);
};

} /* namespace mqtt */

extern "C" {
#endif /* __cplusplus */

/* ── C interface (callable from main.c) ────────────────────────────────────── */

/**
 * @brief  Initialize and start the MQTT manager singleton.
 * @param  broker_uri  Broker URI string. Pass NULL to use MQTT_BROKER_URI default.
 * @return ESP_OK on success, ESP_FAIL on error.
 */
esp_err_t mqtt_manager_init(const char *broker_uri);

/**
 * @brief  Publish all sensor values to their respective MQTT topics.
 *         Silently ignored if the client is not yet connected.
 * @param  temp         Temperature in °C.
 * @param  hum          Relative humidity in %.
 * @param  motion       1 = motion detected, 0 = clear.
 * @param  door         1 = door closed, 0 = open.
 * @param  lux          Brightness percentage 0–100.
 * @param  relay_state  1 = relay on, 0 = off.
 */
void mqtt_manager_publish_sensors(float temp, float hum,
                                  int motion, int door,
                                  int lux, int relay_state);

/**
 * @brief  Returns true when the MQTT client is connected to the broker.
 */
bool mqtt_manager_is_connected(void);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* MQTT_MANAGER_H */
