/**
 * @file    dht11.cpp
 * @brief   DHT11 temperature and humidity sensor — sensor::DhtSensor class implementation + C wrappers
 *
 * Single-wire timing (refer to the DHT11 datasheet):
 *
 *  Host start signal:
 *    DATA pulled low >= 18 ms → pulled high 20~40 μs
 *
 *  DHT11 response:
 *    Pulled low 80 μs → pulled high 80 μs → start transmitting 40-bit data
 *
 *  Bit encoding:
 *    "0": 50 μs low level + 26~28 μs high level
 *    "1": 50 μs low level + 70 μs high level
 *    Bit value is determined by the duration of the high level; threshold ~40 μs
 *
 *  Data format (40 bits):
 *    [39:32] humidity integer  [31:24] humidity fraction (always 0 for DHT11)
 *    [23:16] temperature integer [15:8] temperature fraction (always 0 for DHT11)
 *    [ 7: 0] checksum = lower 8 bits of the sum of the first four bytes
 */

#include <cstring>
#include "dht11.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/portmacro.h"
#include "rom/ets_sys.h"

static const char *TAG = "dht11";

/* Timeout for waiting on a bus level change (μs).
 * DHT11 response signal is at most 80 μs; 200 μs gives sufficient margin. */
#define DHT11_TIMEOUT_US  200

/* ════════════════════════════════════════════════════════════════════════════
 * sensor::DhtSensor implementation
 * ════════════════════════════════════════════════════════════════════════════ */

namespace sensor {

/* ── Private helper ──────────────────────────────────────────────────────── */

int DhtSensor::wait_for_level(int level) const
{
    int elapsed = 0;
    while (gpio_get_level(m_gpio) != level) {
        if (elapsed >= DHT11_TIMEOUT_US) {
            return -1; /* timeout */
        }
        ets_delay_us(1);
        elapsed++;
    }
    return elapsed;
}

/* ── init ────────────────────────────────────────────────────────────────── */

esp_err_t DhtSensor::init(gpio_num_t gpio_num)
{
    m_gpio = gpio_num;

    /* Open-drain: can drive low; pull-up holds high when released */
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << gpio_num),
        .mode         = GPIO_MODE_INPUT_OUTPUT_OD,
        .pull_up_en   = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };

    esp_err_t ret = gpio_config(&io_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "gpio_config failed: %d", ret);
        return ESP_FAIL;
    }

    /* Pull bus high and wait for DHT11 to stabilise after power-on (needs ~1 s) */
    gpio_set_level(m_gpio, 1);
    vTaskDelay(pdMS_TO_TICKS(1000));

    ESP_LOGI(TAG, "dht11 init ok, gpio=%d", gpio_num);
    return ESP_OK;
}

/* ── read ────────────────────────────────────────────────────────────────── */

esp_err_t DhtSensor::read(dht11_data_t *data)
{
    uint8_t raw[5] = {0}; /* 40 bits = 5 bytes */

    /* ---- 1. Host sends start signal ---- */
    gpio_set_level(m_gpio, 0);
    vTaskDelay(pdMS_TO_TICKS(20)); /* 20 ms — satisfies the >= 18 ms requirement */

    /* ---- 2. Disable interrupts before the timing-critical section ---- */
    portDISABLE_INTERRUPTS();

    /* Release the bus: switch to pure input so the pull-up takes effect cleanly */
    gpio_set_direction(m_gpio, GPIO_MODE_INPUT);
    ets_delay_us(30);

    /* ---- 3. Wait for DHT11 response ---- */
    if (wait_for_level(0) < 0) {
        portENABLE_INTERRUPTS();
        gpio_set_direction(m_gpio, GPIO_MODE_INPUT_OUTPUT_OD);
        gpio_set_level(m_gpio, 1);
        ESP_LOGE(TAG, "timeout waiting for DHT11 response low");
        return ESP_ERR_TIMEOUT;
    }
    if (wait_for_level(1) < 0) {
        portENABLE_INTERRUPTS();
        gpio_set_direction(m_gpio, GPIO_MODE_INPUT_OUTPUT_OD);
        gpio_set_level(m_gpio, 1);
        ESP_LOGE(TAG, "timeout waiting for DHT11 response high");
        return ESP_ERR_TIMEOUT;
    }

    /* ---- 4. Read 40 bits of data ---- */
    for (int i = 0; i < 40; i++) {
        /* Each bit starts with a 50 μs low level */
        if (wait_for_level(0) < 0) {
            portENABLE_INTERRUPTS();
            gpio_set_direction(m_gpio, GPIO_MODE_INPUT_OUTPUT_OD);
            gpio_set_level(m_gpio, 1);
            ESP_LOGE(TAG, "timeout at bit %d low", i);
            return ESP_ERR_TIMEOUT;
        }
        if (wait_for_level(1) < 0) {
            portENABLE_INTERRUPTS();
            gpio_set_direction(m_gpio, GPIO_MODE_INPUT_OUTPUT_OD);
            gpio_set_level(m_gpio, 1);
            ESP_LOGE(TAG, "timeout at bit %d high start", i);
            return ESP_ERR_TIMEOUT;
        }

        ets_delay_us(40); /* sample after 40 μs */

        /* HIGH level still present → bit 1; line has gone low → bit 0 */
        raw[i / 8] <<= 1;
        if (gpio_get_level(m_gpio) == 1) {
            raw[i / 8] |= 1;
        }
    }

    portENABLE_INTERRUPTS();

    /* Restore open-drain output, pull bus high ready for next communication */
    gpio_set_direction(m_gpio, GPIO_MODE_INPUT_OUTPUT_OD);
    gpio_set_level(m_gpio, 1);

    /* ---- 5. Checksum verification ---- */
    uint8_t checksum = raw[0] + raw[1] + raw[2] + raw[3];
    if (checksum != raw[4]) {
        ESP_LOGE(TAG, "checksum error: calc=0x%02X recv=0x%02X", checksum, raw[4]);
        return ESP_ERR_INVALID_CRC;
    }

    /* ---- 6. Parse data (DHT11 fractional parts raw[1] and raw[3] are always 0) ---- */
    data->humidity    = static_cast<float>(raw[0]);
    data->temperature = static_cast<float>(raw[2]);

    ESP_LOGI(TAG, "read ok: temp=%.1f humi=%.1f", data->temperature, data->humidity);
    return ESP_OK;
}

} /* namespace sensor */

/* ════════════════════════════════════════════════════════════════════════════
 * C wrapper — file-scoped singleton
 * ════════════════════════════════════════════════════════════════════════════ */

static sensor::DhtSensor s_dht;

esp_err_t dht11_init(gpio_num_t gpio_num) { return s_dht.init(gpio_num); }
esp_err_t dht11_read(dht11_data_t *data)  { return s_dht.read(data); }
