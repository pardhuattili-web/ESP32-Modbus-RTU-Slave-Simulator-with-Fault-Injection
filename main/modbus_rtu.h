#ifndef MODBUS_RTU_H
#define MODBUS_RTU_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#include "register_map.h"

#define MODBUS_MAX_ADU 256U

typedef enum {
    MODBUS_RESULT_IGNORE = 0,
    MODBUS_RESULT_RESPONSE,
    MODBUS_RESULT_DROP,
    MODBUS_RESULT_BAD_FRAME
} modbus_result_t;

typedef struct {
    modbus_result_t result;
    size_t response_length;
    uint16_t delay_ms;
    bool corrupt_crc;
} modbus_response_t;

modbus_response_t modbus_process_request(modbus_device_t *device,
                                         const uint8_t *request,
                                         size_t request_length,
                                         uint8_t *response,
                                         size_t response_capacity);

#endif
