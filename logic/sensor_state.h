/**
 * @file    sensor_state.h
 * @brief   Shared digital sensor state — thread-safe storage for io_task outputs
 *
 * io_task writes motion and door state every 100 ms.
 * sensor_task reads them every 2 s to include in MQTT publish.
 * Atomic int operations on single-word values are safe on ESP32 without a mutex.
 *
 * Dependencies: none
 */

#ifndef SENSOR_STATE_H
#define SENSOR_STATE_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief  Update the shared motion state.
 * @param  detected  1 = motion detected, 0 = clear.
 */
void sensor_state_set_motion(int detected);

/**
 * @brief  Update the shared door state.
 * @param  closed  1 = door closed, 0 = door open.
 */
void sensor_state_set_door(int closed);

/**
 * @brief  Read the latest motion state.
 * @return 1 = motion detected, 0 = clear.
 */
int sensor_state_get_motion(void);

/**
 * @brief  Read the latest door state.
 * @return 1 = door closed, 0 = open.
 */
int sensor_state_get_door(void);

#ifdef __cplusplus
}
#endif

#endif /* SENSOR_STATE_H */
