#ifndef FAULT_INJECTION_H
#define FAULT_INJECTION_H
#include <stdbool.h>
#include <stdint.h>
#include "register_map.h"
typedef enum { FAULT_DROP_NEXT=0, FAULT_DELAY_NEXT, FAULT_CORRUPT_CRC_NEXT } fault_mode_t;
bool fault_injection_arm(modbus_device_t *device, fault_mode_t mode, uint16_t delay_ms);
#endif
