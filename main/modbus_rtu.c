#include "modbus_rtu.h"
#include "modbus_crc.h"

#include <stddef.h>
#include <string.h>

#define FC_READ_HOLDING 0x03U
#define FC_READ_INPUT 0x04U
#define FC_WRITE_SINGLE 0x06U
#define FC_WRITE_MULTIPLE 0x10U
#define EX_ILLEGAL_FUNCTION 0x01U
#define EX_ILLEGAL_ADDRESS 0x02U
#define EX_ILLEGAL_VALUE 0x03U
#define MAX_READ_REGISTERS 125U
#define MAX_WRITE_REGISTERS 123U

static void append_crc(uint8_t *frame, size_t *length)
{
    const uint16_t crc = modbus_crc16(frame, *length);
    frame[(*length)++] = (uint8_t)(crc & 0xFFU);
    frame[(*length)++] = (uint8_t)(crc >> 8U);
}

static modbus_response_t make_exception(modbus_device_t *device, uint8_t address,
                                        uint8_t function, uint8_t exception,
                                        uint8_t *response, size_t capacity)
{
    modbus_response_t out = {MODBUS_RESULT_BAD_FRAME, 0U, 0U, false};
    if (capacity < 5U) {
        return out;
    }
    response[0] = address;
    response[1] = (uint8_t)(function | 0x80U);
    response[2] = exception;
    out.response_length = 3U;
    append_crc(response, &out.response_length);
    out.result = MODBUS_RESULT_RESPONSE;
    if (device != NULL) {
        device->exceptions_sent++;
    }
    return out;
}

static modbus_response_t apply_faults(modbus_device_t *device, modbus_response_t out,
                                      uint8_t *response)
{
    if (out.result != MODBUS_RESULT_RESPONSE || out.response_length < 5U) {
        return out;
    }
    if (device->drop_next) {
        device->drop_next = false;
        device->faults_injected++;
        out.result = MODBUS_RESULT_DROP;
        out.response_length = 0U;
        return out;
    }
    if (device->delay_next) {
        device->delay_next = false;
        device->faults_injected++;
        out.delay_ms = device->delay_ms;
    }
    if (device->corrupt_crc_next) {
        device->corrupt_crc_next = false;
        device->faults_injected++;
        out.corrupt_crc = true;
        response[out.response_length - 1U] ^= 0xFFU;
    }
    return out;
}

modbus_response_t modbus_process_request(modbus_device_t *device,
                                         const uint8_t *request,
                                         size_t request_length,
                                         uint8_t *response,
                                         size_t response_capacity)
{
    modbus_response_t out = {MODBUS_RESULT_BAD_FRAME, 0U, 0U, false};
    if (device == NULL || request == NULL || response == NULL || request_length < 4U ||
        request_length > MODBUS_MAX_ADU) {
        return out;
    }

    device->frames_received++;
    const uint16_t received_crc = (uint16_t)request[request_length - 2U] |
                                  (uint16_t)((uint16_t)request[request_length - 1U] << 8U);
    if (modbus_crc16(request, request_length - 2U) != received_crc) {
        device->crc_errors++;
        return out;
    }

    const uint8_t address = request[0];
    const uint8_t function = request[1];
    if (address != device->slave_address && address != 0U) {
        return (modbus_response_t){MODBUS_RESULT_IGNORE, 0U, 0U, false};
    }
    device->frames_valid++;

    /* Broadcast has no response in RTU. This test simulator skips broadcast writes. */
    if (address == 0U) {
        return (modbus_response_t){MODBUS_RESULT_IGNORE, 0U, 0U, false};
    }
    if (response_capacity < 5U) {
        return out;
    }

    const uint16_t start = (uint16_t)((uint16_t)request[2] << 8U) | request[3];
    size_t response_length = 0U;
    response[0] = address;
    response[1] = function;

    if (function == FC_READ_HOLDING || function == FC_READ_INPUT) {
        if (request_length != 8U) {
            return make_exception(device, address, function, EX_ILLEGAL_VALUE, response, response_capacity);
        }
        const uint16_t quantity = (uint16_t)((uint16_t)request[4] << 8U) | request[5];
        if (quantity == 0U || quantity > MAX_READ_REGISTERS) {
            return make_exception(device, address, function, EX_ILLEGAL_VALUE, response, response_capacity);
        }
        const uint16_t limit = function == FC_READ_HOLDING ? MODBUS_HOLDING_COUNT : MODBUS_INPUT_COUNT;
        if (start >= limit || quantity > (uint16_t)(limit - start)) {
            return make_exception(device, address, function, EX_ILLEGAL_ADDRESS, response, response_capacity);
        }
        if (response_capacity < (size_t)(5U + 2U * quantity)) {
            return out;
        }
        response[2] = (uint8_t)(quantity * 2U);
        response_length = 3U;
        for (uint16_t i = 0U; i < quantity; ++i) {
            uint16_t value = 0U;
            const bool ok = function == FC_READ_HOLDING
                ? modbus_holding_read(device, (uint16_t)(start + i), &value)
                : modbus_input_read(device, (uint16_t)(start + i), &value);
            if (!ok) {
                return make_exception(device, address, function, EX_ILLEGAL_ADDRESS, response, response_capacity);
            }
            response[response_length++] = (uint8_t)(value >> 8U);
            response[response_length++] = (uint8_t)value;
        }
    } else if (function == FC_WRITE_SINGLE) {
        if (request_length != 8U) {
            return make_exception(device, address, function, EX_ILLEGAL_VALUE, response, response_capacity);
        }
        const uint16_t value = (uint16_t)((uint16_t)request[4] << 8U) | request[5];
        if (start >= MODBUS_HOLDING_COUNT) {
            return make_exception(device, address, function, EX_ILLEGAL_ADDRESS, response, response_capacity);
        }
        if (!modbus_holding_write(device, start, value)) {
            return make_exception(device, address, function, EX_ILLEGAL_VALUE, response, response_capacity);
        }
        memcpy(response, request, 6U);
        response_length = 6U;
    } else if (function == FC_WRITE_MULTIPLE) {
        if (request_length < 9U) {
            return make_exception(device, address, function, EX_ILLEGAL_VALUE, response, response_capacity);
        }
        const uint16_t quantity = (uint16_t)((uint16_t)request[4] << 8U) | request[5];
        const uint8_t byte_count = request[6];
        if (quantity == 0U || quantity > MAX_WRITE_REGISTERS ||
            byte_count != (uint8_t)(quantity * 2U) ||
            request_length != (size_t)(9U + byte_count)) {
            return make_exception(device, address, function, EX_ILLEGAL_VALUE, response, response_capacity);
        }
        if (start >= MODBUS_HOLDING_COUNT || quantity > (uint16_t)(MODBUS_HOLDING_COUNT - start)) {
            return make_exception(device, address, function, EX_ILLEGAL_ADDRESS, response, response_capacity);
        }
        /* Validate all registers first, so an invalid value cannot leave a partial write. */
        for (uint16_t i = 0U; i < quantity; ++i) {
            const uint16_t value = (uint16_t)((uint16_t)request[7U + 2U * i] << 8U) |
                                   request[8U + 2U * i];
            if (start + i == 1U && value > 2U) {
                return make_exception(device, address, function, EX_ILLEGAL_VALUE, response, response_capacity);
            }
        }
        for (uint16_t i = 0U; i < quantity; ++i) {
            const uint16_t value = (uint16_t)((uint16_t)request[7U + 2U * i] << 8U) |
                                   request[8U + 2U * i];
            (void)modbus_holding_write(device, (uint16_t)(start + i), value);
        }
        memcpy(response, request, 6U);
        response_length = 6U;
    } else {
        return make_exception(device, address, function, EX_ILLEGAL_FUNCTION, response, response_capacity);
    }

    append_crc(response, &response_length);
    out.result = MODBUS_RESULT_RESPONSE;
    out.response_length = response_length;
    return apply_faults(device, out, response);
}
