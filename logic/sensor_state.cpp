/**
 * @file    sensor_state.cpp
 * @brief   Global sensor data bus implementation
 *
 * Single-word volatile variables are atomically read/written on ESP32 (Xtensa LX6).
 * float is 32-bit on Xtensa, so float reads/writes are also single-word atomic.
 */

#include "sensor_state.h"

static volatile int   s_motion      = 0;
static volatile int   s_door        = 0;
static volatile float s_temperature = 0.0f;
static volatile float s_humidity    = 0.0f;
static volatile int   s_lux         = 0;

void sensor_state_set_motion(int v)      { s_motion      = v; }
void sensor_state_set_door(int v)        { s_door        = v; }
void sensor_state_set_temperature(float v) { s_temperature = v; }
void sensor_state_set_humidity(float v)  { s_humidity    = v; }
void sensor_state_set_lux(int v)         { s_lux         = v; }

int   sensor_state_get_motion(void)      { return s_motion;      }
int   sensor_state_get_door(void)        { return s_door;        }
float sensor_state_get_temperature(void) { return s_temperature; }
float sensor_state_get_humidity(void)    { return s_humidity;    }
int   sensor_state_get_lux(void)         { return s_lux;         }
