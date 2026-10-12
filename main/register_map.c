#include "register_map.h"

#include <stddef.h>
#include <string.h>

void modbus_device_init(modbus_device_t *device, uint8_t slave_address)
{
    if (device == NULL) return;
    memset(device, 0, sizeof(*device));
    device->slave_address = (slave_address >= 1U && slave_address <= 247U) ? slave_address : 1U;
    device->holding[0] = 250U;
    device->holding[1] = 0U;
    device->holding[2] = 0U;
    device->delay_ms = 100U;
    device->input[0] = 237U;
    device->input[1] = 0x0100U;
}

void modbus_device_update_inputs(modbus_device_t *device, uint32_t uptime_seconds)
{
    if (device == NULL) return;
    device->uptime_seconds = uptime_seconds;
    device->input[0] = (uint16_t)(237U + ((uptime_seconds / 3U) % 12U));
    device->input[1] = 0x0100U;
    device->input[2] = (uint16_t)(uptime_seconds & 0xFFFFU);
    device->input[3] = (uint16_t)((device->crc_errors > 0U ? 1U : 0U) |
                                  (device->exceptions_sent > 0U ? 2U : 0U));
}

bool modbus_holding_read(const modbus_device_t *device, uint16_t address, uint16_t *value)
{
    if (device == NULL || value == NULL || address >= MODBUS_HOLDING_COUNT) return false;
    *value = device->holding[address];
    return true;
}

bool modbus_holding_write(modbus_device_t *device, uint16_t address, uint16_t value)
{
    if (device == NULL || address >= MODBUS_HOLDING_COUNT) return false;
    if (address == 1U && value > 2U) return false;
    if (address == 2U) {
        /* Bits are one-shot triggers rather than persistent control flags. */
        const uint16_t mask = (uint16_t)(value & 0x0007U);
        device->holding[address] = 0U;
        device->drop_next = (mask & 0x0001U) != 0U;
        device->delay_next = (mask & 0x0002U) != 0U;
        device->corrupt_crc_next = (mask & 0x0004U) != 0U;
        return true;
    }
    device->holding[address] = value;
    return true;
}

bool modbus_input_read(const modbus_device_t *device, uint16_t address, uint16_t *value)
{
    if (device == NULL || value == NULL || address >= MODBUS_INPUT_COUNT) return false;
    *value = device->input[address];
    return true;
}
