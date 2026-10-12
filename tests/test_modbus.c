#include "modbus_crc.h"
#include "modbus_rtu.h"
#include "register_map.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static size_t make_request(uint8_t *frame, uint8_t address, uint8_t function,
                           uint16_t start, uint16_t quantity_or_value)
{
    frame[0] = address;
    frame[1] = function;
    frame[2] = (uint8_t)(start >> 8U);
    frame[3] = (uint8_t)start;
    frame[4] = (uint8_t)(quantity_or_value >> 8U);
    frame[5] = (uint8_t)quantity_or_value;
    const uint16_t crc = modbus_crc16(frame, 6U);
    frame[6] = (uint8_t)crc;
    frame[7] = (uint8_t)(crc >> 8U);
    return 8U;
}

static void test_crc_vector(void)
{
    static const uint8_t payload[] = "123456789";
    assert(modbus_crc16(payload, 9U) == 0x4B37U);
    assert(modbus_crc16(NULL, 0U) == 0xFFFFU);
}

static void test_read_holding(void)
{
    modbus_device_t device;
    uint8_t request[16], response[MODBUS_MAX_ADU];
    modbus_device_init(&device, 1U);
    size_t length = make_request(request, 1U, 0x03U, 0U, 2U);
    modbus_response_t result = modbus_process_request(&device, request, length, response, sizeof(response));
    assert(result.result == MODBUS_RESULT_RESPONSE);
    assert(result.response_length == 9U);
    assert(response[0] == 1U && response[1] == 0x03U && response[2] == 4U);
    assert(response[3] == 0U && response[4] == 250U);
    const uint16_t response_crc = (uint16_t)response[result.response_length - 2U] |
        (uint16_t)((uint16_t)response[result.response_length - 1U] << 8U);
    assert(modbus_crc16(response, result.response_length - 2U) == response_crc);
}

static void test_invalid_address(void)
{
    modbus_device_t device;
    uint8_t request[16], response[MODBUS_MAX_ADU];
    modbus_device_init(&device, 1U);
    size_t length = make_request(request, 1U, 0x03U, 15U, 2U);
    modbus_response_t result = modbus_process_request(&device, request, length, response, sizeof(response));
    assert(result.result == MODBUS_RESULT_RESPONSE);
    assert(response[1] == 0x83U && response[2] == 0x02U);
}

static void test_write_single(void)
{
    modbus_device_t device;
    uint8_t request[16], response[MODBUS_MAX_ADU];
    modbus_device_init(&device, 1U);
    size_t length = make_request(request, 1U, 0x06U, 0U, 321U);
    modbus_response_t result = modbus_process_request(&device, request, length, response, sizeof(response));
    uint16_t value = 0U;
    assert(result.result == MODBUS_RESULT_RESPONSE);
    assert(modbus_holding_read(&device, 0U, &value) && value == 321U);
    assert(memcmp(request, response, 6U) == 0);
}

static void test_crc_failure_and_slave_filter(void)
{
    modbus_device_t device;
    uint8_t request[16], response[MODBUS_MAX_ADU];
    modbus_device_init(&device, 1U);
    size_t length = make_request(request, 1U, 0x03U, 0U, 1U);
    request[2] ^= 0x80U;
    modbus_response_t result = modbus_process_request(&device, request, length, response, sizeof(response));
    assert(result.result == MODBUS_RESULT_BAD_FRAME && device.crc_errors == 1U);

    length = make_request(request, 2U, 0x03U, 0U, 1U);
    result = modbus_process_request(&device, request, length, response, sizeof(response));
    assert(result.result == MODBUS_RESULT_IGNORE);
}

static void test_drop_fault(void)
{
    modbus_device_t device;
    uint8_t request[16], response[MODBUS_MAX_ADU];
    modbus_device_init(&device, 1U);
    assert(modbus_holding_write(&device, 2U, 1U));
    size_t length = make_request(request, 1U, 0x03U, 0U, 1U);
    modbus_response_t result = modbus_process_request(&device, request, length, response, sizeof(response));
    assert(result.result == MODBUS_RESULT_DROP);
    assert(result.response_length == 0U && device.faults_injected == 1U);
}

static void test_bad_values(void)
{
    modbus_device_t device;
    uint8_t request[16], response[MODBUS_MAX_ADU];
    modbus_device_init(&device, 1U);
    size_t length = make_request(request, 1U, 0x06U, 1U, 9U);
    modbus_response_t result = modbus_process_request(&device, request, length, response, sizeof(response));
    assert(result.result == MODBUS_RESULT_RESPONSE);
    assert(response[1] == 0x86U && response[2] == 0x03U);
}

int main(void)
{
    test_crc_vector();
    test_read_holding();
    test_invalid_address();
    test_write_single();
    test_crc_failure_and_slave_filter();
    test_drop_fault();
    test_bad_values();
    puts("All Modbus protocol tests passed.");
    return 0;
}
