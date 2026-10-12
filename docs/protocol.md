# Modbus Protocol Reference

Default serial settings: UART2, 19200 baud, 8N1; slave address 1. RTU register addresses are zero-based.

## Supported function codes

| Code | Name | Description |
|---:|---|---|
| 03 | Read Holding Registers | Read holding registers |
| 04 | Read Input Registers | Read input registers |
| 06 | Write Single Register | Update and echo one holding register |
| 16 (0x10) | Write Multiple Registers | Update contiguous holding registers |

The current holding and input banks have 16 entries each. Requests crossing address 15 are rejected.

## Register map

| Address | Bank | Meaning |
|---:|---|---|
| 0 | Holding | Setpoint, default 250 |
| 1 | Holding | Mode: 0 idle, 1 run, 2 fault |
| 2 | Holding | Fault mask: bit 0 drop, bit 1 delay, bit 2 corrupt CRC |
| 0 | Input | Simulated process value |
| 1 | Input | Firmware marker 0x0100 |
| 2 | Input | Uptime seconds low word |
| 3 | Input | Diagnostic flags |

Exception codes: 01 illegal function, 02 illegal data address, 03 illegal data value. RTU CRC is sent low byte first.

Fault-control writes can affect the write acknowledgement itself because a newly armed fault applies to the next response. Test one fault at a time with a dedicated isolated master; never test against a production bus.