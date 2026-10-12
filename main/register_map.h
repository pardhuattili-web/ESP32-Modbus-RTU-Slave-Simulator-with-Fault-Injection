#ifndef MODBUS_REGISTER_MAP_H
#define MODBUS_REGISTER_MAP_H

#include <stdbool.h>
#include <stdint.h>

#define MODBUS_HOLDING_COUNT 16U
#define MODBUS_INPUT_COUNT 16U

typedef struct {
    uint16_t holding[MODBUS_HOLDING_COUNT];
    uint16_t input[MODBUS_INPUT_COUNT];
    uint32_t frames_received;
    uint32_t frames_valid;
    uint32_t crc_errors;
    uint32_t exceptions_sent;
    uint32_t faults_injected;
    uint32_t uptime_seconds;
    uint8_t slave_address;
    bool drop_next;
    bool delay_next;
    bool corrupt_crc_next;
    uint16_t delay_ms;
} modbus_device_t;

void modbus_device_init(modbus_device_t *device, uint8_t slave_address);
void modbus_device_update_inputs(modbus_device_t *device, uint32_t uptime_seconds);
bool modbus_holding_read(const modbus_device_t *device, uint16_t address, uint16_t *value);
bool modbus_holding_write(modbus_device_t *device, uint16_t address, uint16_t value);
bool modbus_input_read(const modbus_device_t *device, uint16_t address, uint16_t *value);

#endif
