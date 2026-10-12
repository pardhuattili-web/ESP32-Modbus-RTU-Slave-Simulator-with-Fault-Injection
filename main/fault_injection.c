#include "fault_injection.h"

bool fault_injection_arm(modbus_device_t *device, fault_mode_t mode, uint16_t delay_ms)
{
    if (device == NULL) {
        return false;
    }
    switch (mode) {
    case FAULT_DROP_NEXT:
        device->drop_next = true;
        break;
    case FAULT_DELAY_NEXT:
        device->delay_next = true;
        device->delay_ms = delay_ms;
        break;
    case FAULT_CORRUPT_CRC_NEXT:
        device->corrupt_crc_next = true;
        break;
    default:
        return false;
    }
    return true;
}
