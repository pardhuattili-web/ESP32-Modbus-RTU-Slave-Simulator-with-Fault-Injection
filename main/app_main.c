#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "modbus_rtu.h"
#include "fault_injection.h"
#include "register_map.h"

#define MODBUS_UART UART_NUM_2
#define MODBUS_TX_GPIO GPIO_NUM_17
#define MODBUS_RX_GPIO GPIO_NUM_16
#define MODBUS_DE_RE_GPIO GPIO_NUM_4
#define MODBUS_BAUDRATE 19200
#define MODBUS_SLAVE_ADDRESS 1U
#define MODBUS_RX_BUFFER 512
#define MODBUS_INTERFRAME_MS 5U

static const char *TAG = "modbus_simulator";
static modbus_device_t s_device;

static void rs485_set_transmit(bool transmit)
{
    gpio_set_level(MODBUS_DE_RE_GPIO, transmit ? 1 : 0);
}

static esp_err_t uart_setup(void)
{
    const uart_config_t config = {
        .baud_rate = MODBUS_BAUDRATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT
    };

    ESP_ERROR_CHECK(uart_driver_install(MODBUS_UART, MODBUS_RX_BUFFER, 0, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(MODBUS_UART, &config));
    ESP_ERROR_CHECK(uart_set_pin(MODBUS_UART, MODBUS_TX_GPIO, MODBUS_RX_GPIO,
                                 UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    ESP_ERROR_CHECK(gpio_reset_pin(MODBUS_DE_RE_GPIO));
    ESP_ERROR_CHECK(gpio_set_direction(MODBUS_DE_RE_GPIO, GPIO_MODE_OUTPUT));
    rs485_set_transmit(false);
    return ESP_OK;
}

static void process_adu(const uint8_t *request, size_t request_len)
{
    uint8_t response[MODBUS_MAX_ADU];
    modbus_response_t result = modbus_process_request(&s_device, request, request_len,
                                                     response, sizeof(response));

    if (result.result == MODBUS_RESULT_BAD_FRAME) {
        ESP_LOGW(TAG, "discarded malformed/CRC-invalid frame (len=%u)",
                 (unsigned)request_len);
        return;
    }
    if (result.result == MODBUS_RESULT_IGNORE) {
        return;
    }
    if (result.result == MODBUS_RESULT_DROP) {
        ESP_LOGW(TAG, "injected dropped response");
        return;
    }
    if (result.result != MODBUS_RESULT_RESPONSE || result.response_length == 0U) {
        return;
    }

    if (result.delay_ms > 0U) {
        vTaskDelay(pdMS_TO_TICKS(result.delay_ms));
    }

    rs485_set_transmit(true);
    /* Give the transceiver time to switch direction before sending. */
    esp_rom_delay_us(20);
    (void)uart_write_bytes(MODBUS_UART, (const char *)response, result.response_length);
    (void)uart_wait_tx_done(MODBUS_UART, pdMS_TO_TICKS(100));
    esp_rom_delay_us(20);
    rs485_set_transmit(false);
}

void app_main(void)
{
    ESP_ERROR_CHECK(uart_setup());
    modbus_device_init(&s_device, MODBUS_SLAVE_ADDRESS);
    ESP_LOGI(TAG, "Modbus RTU slave ready: address=%u baud=%d UART2 TX=%d RX=%d DE/RE=%d",
             MODBUS_SLAVE_ADDRESS, MODBUS_BAUDRATE, MODBUS_TX_GPIO, MODBUS_RX_GPIO,
             MODBUS_DE_RE_GPIO);

    uint8_t chunk[MODBUS_RX_BUFFER];
    uint8_t frame[MODBUS_MAX_ADU];
    size_t frame_len = 0U;
    int64_t last_byte_us = 0;
    int64_t start_us = esp_timer_get_time();

    while (true) {
        const int got = uart_read_bytes(MODBUS_UART, chunk, sizeof(chunk), pdMS_TO_TICKS(10));
        if (got > 0) {
            const int64_t now_us = esp_timer_get_time();
            if (frame_len > 0U && (now_us - last_byte_us) > (int64_t)MODBUS_INTERFRAME_MS * 1000) {
                process_adu(frame, frame_len);
                frame_len = 0U;
            }
            for (int i = 0; i < got; ++i) {
                if (frame_len < sizeof(frame)) {
                    frame[frame_len++] = chunk[i];
                } else {
                    ESP_LOGW(TAG, "oversized frame discarded");
                    frame_len = 0U;
                    break;
                }
            }
            last_byte_us = now_us;
        } else if (frame_len > 0U) {
            const int64_t now_us = esp_timer_get_time();
            if ((now_us - last_byte_us) > (int64_t)MODBUS_INTERFRAME_MS * 1000) {
                process_adu(frame, frame_len);
                frame_len = 0U;
            }
        }

        const uint32_t uptime = (uint32_t)((esp_timer_get_time() - start_us) / 1000000);
        modbus_device_update_inputs(&s_device, uptime);

        static uint32_t last_diag_seconds = 0U;
        if (uptime != 0U && uptime != last_diag_seconds && (uptime % 10U) == 0U) {
            last_diag_seconds = uptime;
            ESP_LOGI(TAG, "uptime=%" PRIu32 " rx=%" PRIu32 " valid=%" PRIu32
                     " crc_err=%" PRIu32 " exceptions=%" PRIu32 " faults=%" PRIu32,
                     uptime, s_device.frames_received, s_device.frames_valid,
                     s_device.crc_errors, s_device.exceptions_sent, s_device.faults_injected);
        }
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}
