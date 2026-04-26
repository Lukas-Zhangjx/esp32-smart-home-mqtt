/**
 * @file    sensor_state.cpp
 * @brief   Shared digital sensor state implementation
 *
 * Single-word volatile variables are atomically read/written on ESP32 (Xtensa LX6),
 * so no mutex is needed for these simple int flags.
 */

#include "sensor_state.h"

static volatile int s_motion = 0;
static volatile int s_door   = 0;

void sensor_state_set_motion(int detected) { s_motion = detected; }
void sensor_state_set_door(int closed)     { s_door   = closed;   }
int  sensor_state_get_motion(void)         { return s_motion;     }
int  sensor_state_get_door(void)           { return s_door;       }
