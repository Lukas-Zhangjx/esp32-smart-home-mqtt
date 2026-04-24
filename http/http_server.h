/**
 * @file    http_server.h
 * @brief   HTTP server module public interface
 *
 * Implemented on top of the ESP-IDF esp_http_server component.
 * Provides the following functionality:
 *   - Serve the index.html static page (GET /)
 *   - Sensor data JSON endpoint (GET /api/sensors)
 *   - Relay control JSON endpoint (POST /api/relay)
 *
 * Dependencies:
 *   - esp_http_server
 *   - dht11 driver (sensor/dht11.h)
 *   - relay module (stub, pending implementation)
 */

#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_err.h"
#include "dht11.h"

/**
 * @brief  Start the HTTP server and register all URI handlers
 *
 * Should be called after a successful WiFi connection (IP address obtained).
 *
 * @return ESP_OK      started successfully
 * @return ESP_FAIL    failed to start
 */
esp_err_t http_server_start(void);

/**
 * @brief  Stop the HTTP server and release resources
 */
void http_server_stop(void);

/**
 * @brief  Update the sensor cache with a pre-read DHT11 result.
 *
 * sensor_task calls dht11_read() once and passes the result here so the same
 * data can also be forwarded to MQTT without triggering a second read.
 * A zero-value result (failed read) is silently ignored — the last good cache
 * value is retained.
 *
 * @param data  DHT11 data struct from the caller.
 */
void http_server_update_sensor(dht11_data_t data);

/**
 * @brief  Update the obstacle detection state cache (door/window sensor)
 *
 * Should be called from io_task when a state change is detected.
 *
 * @param detected  1 = detected (door closed), 0 = not detected (door open)
 */
void http_server_update_obstacle(int detected);

/**
 * @brief  Update the IR sensor state cache (human / motion detection)
 *
 * Should be called from io_task when a state change is detected.
 *
 * @param detected  1 = motion detected, 0 = no motion
 */
void http_server_update_ir(int detected);

/**
 * @brief  Update the light sensor cache
 *
 * @param percent  Light intensity percentage 0-100 (100 = brightest)
 * @param raw      Raw ADC value 0-4095 (for calibration)
 * @param digital  1 = bright, 0 = dark
 */
void http_server_update_light(int percent, int raw, int digital);

#ifdef __cplusplus
}
#endif

#endif /* HTTP_SERVER_H */
