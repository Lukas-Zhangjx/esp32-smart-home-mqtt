/**
 * @file    sensor_state.h
 * @brief   Global sensor data bus — shared state between time-based tasks
 *
 * Writers (100 ms task):  obstacle_run(), ir_sensor_run()
 * Writers (2000 ms task): dht11_run(), light_sensor_run()
 * Reader  (2000 ms task): mqtt_manager_run()
 *
 * All fields are single-word volatile variables.  Single-word reads/writes are
 * atomic on ESP32 (Xtensa LX6), so no mutex is required.
 */

#ifndef SENSOR_STATE_H
#define SENSOR_STATE_H

#ifdef __cplusplus
extern "C" {
#endif

/* ── Writers ─────────────────────────────────────────────────────────────── */

void sensor_state_set_motion(int detected);   /* 1 = motion, 0 = clear       */
void sensor_state_set_door(int closed);       /* 1 = closed, 0 = open        */
void sensor_state_set_temperature(float val); /* °C                          */
void sensor_state_set_humidity(float val);    /* %RH                         */
void sensor_state_set_lux(int percent);       /* 0–100                       */

/* ── Readers ─────────────────────────────────────────────────────────────── */

int   sensor_state_get_motion(void);
int   sensor_state_get_door(void);
float sensor_state_get_temperature(void);
float sensor_state_get_humidity(void);
int   sensor_state_get_lux(void);

#ifdef __cplusplus
}
#endif

#endif /* SENSOR_STATE_H */
