/**
 * @file    dht11.c
 * @brief   DHT11 temperature and humidity sensor driver implementation
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

#include <string.h>
#include "dht11.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/portmacro.h"
#include "rom/ets_sys.h"

static const char *TAG = "dht11";

/* Stores the GPIO number configured at initialization */
static gpio_num_t s_gpio_num = GPIO_NUM_NC;

/* Timeout for waiting on a bus level change (μs).
 * DHT11 response signal is at most 80 μs; set to 200 μs for sufficient margin. */
#define DHT11_TIMEOUT_US  200


/**
 * @brief  Wait for the DATA line to reach the specified level; returns elapsed
 *         time in μs, or -1 on timeout.
 *
 * @param level  Expected level (0 or 1)
 * @return Elapsed time (μs), or -1 on timeout
 */
static int wait_for_level(int level)
{
    int elapsed = 0;
    while (gpio_get_level(s_gpio_num) != level) {
        if (elapsed >= DHT11_TIMEOUT_US) {
            return -1; /* timeout */
        }
        ets_delay_us(1);
        elapsed++;
    }
    return elapsed;
}


/**
 * @brief  Initialize DHT11; configure the DATA pin as open-drain output
 *
 * @param gpio_num  DATA pin number
 * @return ESP_OK / ESP_FAIL
 */
esp_err_t dht11_init(gpio_num_t gpio_num)
{
    s_gpio_num = gpio_num;

    /* Configure as open-drain: can drive low; when released, pull-up resistor pulls high */
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << gpio_num),
        .mode         = GPIO_MODE_INPUT_OUTPUT_OD, /* open-drain, readable and writable */
        .pull_up_en   = GPIO_PULLUP_ENABLE,        /* enable internal pull-up */
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };

    esp_err_t ret = gpio_config(&io_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "gpio_config failed: %d", ret);
        return ESP_FAIL;
    }

    /* Pull the bus high after initialization and wait for DHT11 to stabilize (needs 1 s after power-on) */
    gpio_set_level(s_gpio_num, 1);
    vTaskDelay(pdMS_TO_TICKS(1000));

    ESP_LOGI(TAG, "dht11 init ok, gpio=%d", gpio_num);
    return ESP_OK;
}


/**
 * @brief  Read one set of temperature and humidity data from DHT11
 *
 * @param data  Output result
 * @return ESP_OK / ESP_ERR_TIMEOUT / ESP_ERR_INVALID_CRC
 */
esp_err_t dht11_read(dht11_data_t *data)
{
    uint8_t raw[5] = {0}; /* 40 bits = 5 bytes of raw data */

    /* ---- 1. Host sends start signal ---- */
    /* Pull low >= 18 ms to notify DHT11 to begin communication */
    gpio_set_level(s_gpio_num, 0);
    vTaskDelay(pdMS_TO_TICKS(20)); /* 20 ms, exceeds the minimum 18 ms requirement */

    /* ---- 2. Disable interrupts before the timing-critical section ---- */
    portDISABLE_INTERRUPTS();

    /* Release the bus: switch to pure input mode to completely remove the output driver;
     * the pull-up resistor pulls the line high.
     * open-drain set(1) may not release cleanly in some cases; pure input is most reliable. */
    gpio_set_direction(s_gpio_num, GPIO_MODE_INPUT);
    ets_delay_us(30);

    /* ---- 3. Wait for DHT11 response ---- */
    /* Response: first pulled low for 80 μs */
    if (wait_for_level(0) < 0) {
        portENABLE_INTERRUPTS();
        gpio_set_direction(s_gpio_num, GPIO_MODE_INPUT_OUTPUT_OD);
        gpio_set_level(s_gpio_num, 1);
        ESP_LOGE(TAG, "timeout waiting for DHT11 response low");
        return ESP_ERR_TIMEOUT;
    }
    /* Then pulled high for 80 μs */
    if (wait_for_level(1) < 0) {
        portENABLE_INTERRUPTS();
        gpio_set_direction(s_gpio_num, GPIO_MODE_INPUT_OUTPUT_OD);
        gpio_set_level(s_gpio_num, 1);
        ESP_LOGE(TAG, "timeout waiting for DHT11 response high");
        return ESP_ERR_TIMEOUT;
    }

    /* ---- 3. Read 40 bits of data ---- */
    for (int i = 0; i < 40; i++) {
        /* Each bit starts with 50 μs low level */
        if (wait_for_level(0) < 0) {
            portENABLE_INTERRUPTS();
            gpio_set_direction(s_gpio_num, GPIO_MODE_INPUT_OUTPUT_OD);
            gpio_set_level(s_gpio_num, 1);
            ESP_LOGE(TAG, "timeout at bit %d low", i);
            return ESP_ERR_TIMEOUT;
        }
        /* High-level duration determines the bit value:
         *   < 40 μs  → bit 0
         *   >= 40 μs → bit 1 */
        if (wait_for_level(1) < 0) {
            portENABLE_INTERRUPTS();
            gpio_set_direction(s_gpio_num, GPIO_MODE_INPUT_OUTPUT_OD);
            gpio_set_level(s_gpio_num, 1);
            ESP_LOGE(TAG, "timeout at bit %d high start", i);
            return ESP_ERR_TIMEOUT;
        }

        ets_delay_us(40); /* wait 40 μs, then sample */

        /* If the high level is still present → bit 1; if it has ended → bit 0 */
        raw[i / 8] <<= 1;
        if (gpio_get_level(s_gpio_num) == 1) {
            raw[i / 8] |= 1;
        }
    }

    portENABLE_INTERRUPTS();

    /* Restore open-drain output, pull bus high, ready for next communication */
    gpio_set_direction(s_gpio_num, GPIO_MODE_INPUT_OUTPUT_OD);
    gpio_set_level(s_gpio_num, 1);

    /* ---- 4. Checksum verification ---- */
    uint8_t checksum = raw[0] + raw[1] + raw[2] + raw[3];
    if (checksum != raw[4]) {
        ESP_LOGE(TAG, "checksum error: calc=0x%02X recv=0x%02X", checksum, raw[4]);
        return ESP_ERR_INVALID_CRC;
    }

    /* ---- 5. Parse data ---- */
    /* DHT11 fractional parts (raw[1], raw[3]) are always 0; use integer parts directly */
    data->humidity    = (float)raw[0];
    data->temperature = (float)raw[2];

    ESP_LOGI(TAG, "read ok: temp=%.1f humi=%.1f", data->temperature, data->humidity);
    return ESP_OK;
}
