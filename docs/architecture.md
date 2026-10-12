# Architecture

## Modules
- `app_main.c`: initializes UART2/GPIO, separates RTU frames using an idle timeout, dispatches requests, and drives RS-485 direction.
- `modbus_crc.c`: CRC-16/Modbus calculation; RTU transmits low byte first.
- `modbus_rtu.c`: address filtering, request validation, FC03/04/06/16 handling, exception responses, response integrity, and fault hooks.
- `register_map.c`: simulated holding/input register banks and diagnostics.
- `tools/modbus_test_harness.py`: host-side serial read smoke test.

## Request flow
1. UART2 collects bytes until a quiet period is observed.
2. The parser validates frame length and CRC.
3. Slave address and function are checked.
4. Quantity, data length, and address limits are validated.
5. A response is generated and CRC appended.
6. Optional one-shot fault injection drops, delays, or corrupts one response.
7. Firmware toggles DE/RE, transmits, waits for UART TX completion, then returns to receive mode.

## Limitations
- A fixed 5 ms frame gap is used for this demonstrator instead of baud-dependent t1.5/t3.5 timing.
- The receive loop does not yet use the ESP-IDF UART event queue for robust overflow/framing-error recovery.
- Host tests validate protocol logic but not electrical timing, transceiver turnaround, or bus contention.
- Broadcast requests are ignored.
- Fault injection is for an isolated test bench only.