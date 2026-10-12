# ESP32 Modbus RTU Slave Simulator with Fault Injection

![Platform](https://img.shields.io/badge/platform-ESP32-E7352C) ![Framework](https://img.shields.io/badge/framework-ESP--IDF-blue) ![Protocol](https://img.shields.io/badge/protocol-Modbus%20RTU-1976D2) ![CI](https://img.shields.io/badge/tests-C%20%2B%20Python-informational)

![Platform](https://img.shields.io/badge/platform-ESP32-E7352C)
![Framework](https://img.shields.io/badge/framework-ESP--IDF-blue)
![Protocol](https://img.shields.io/badge/protocol-Modbus%20RTU-1976D2)
![Tests](https://img.shields.io/badge/CI-C%20%2B%20Python-informational)

A compact industrial-protocol test device for learning and demonstrating Modbus RTU over RS-485. The ESP32 firmware implements a small Modbus slave register map, supports standard register operations and exceptions, tracks diagnostics, and exposes controlled fault injection for testing a Modbus master.

> **Status:** portable protocol core, ESP-IDF firmware skeleton, host unit tests, and documentation are provided. Verify the ESP-IDF cross-build and physical serial behavior using the actual ESP32 and RS-485 transceiver before claiming hardware validation.

## Features

- Modbus RTU slave address configurable in firmware.
- Function codes 03 (Read Holding Registers), 04 (Read Input Registers), 06 (Write Single Register), and 16 (Write Multiple Registers).
- CRC-16/Modbus validation; low CRC byte transmitted first.
- Validation of request length, register ranges, and quantities.
- Modbus exception responses for unsupported functions, invalid addresses, and invalid values.
- UART2 half-duplex RS-485 direction control.
- One-shot fault hooks: dropped response, delayed response, and corrupted response CRC.
- Diagnostic counters for received frames, valid frames, CRC failures, exceptions, and injected faults.
- Portable C protocol tests and a Python serial smoke-test tool.
- Wiring, architecture, protocol, test plan, and GitHub Actions CI.

## Hardware

| Component | Quantity | Purpose |
|---|---:|---|
| ESP32 DevKit / ESP32-WROOM-32 | 1 | Slave firmware |
| 3.3 V logic-compatible RS-485 transceiver | 1 | Differential UART interface |
| USB-to-RS-485 adapter | 1 | PC/host master |
| USB data cable | 1 | Flashing and logs |
| Twisted pair | As needed | A/B bus |
| Termination resistor | As required | Bus-end termination |

**Electrical safety:** some classic MAX485 modules operate at 5 V and may drive 5 V logic on RO. ESP32 GPIO is not 5 V tolerant. Use a transceiver with 3.3 V-compatible logic or a verified level shifter. Do not connect A/B directly to ESP32 GPIO.

## Default pins

| ESP32 | Transceiver | Purpose |
|---|---|---|
| GPIO17 | DI | UART2 TX |
| GPIO16 | RO | UART2 RX |
| GPIO4 | DE and /RE tied | Direction control |
| GND | GND | Common reference |
| A/B | A/B to adapter | Differential bus |

Firmware defaults: UART2, 19200 baud, 8N1, slave address 1. Always verify A/B polarity and pin names from the actual module documentation.

## Repository layout

```text
.github/workflows/ci.yml     Host CI
main/app_main.c               ESP-IDF UART and RS-485 loop
main/modbus_crc.c             CRC-16/Modbus
main/modbus_rtu.c             Request parser and response builder
main/register_map.c           Simulated register banks
main/fault_injection.c        Fault-control helper
tests/test_modbus.c           Portable C unit tests
tests/test_harness_unit.py    Python CRC/frame tests
tools/modbus_test_harness.py  Serial test utility
docs/architecture.md          Module design and data flow
docs/wiring.md                Pinout and electrical notes
docs/protocol.md              Register/function reference
docs/test_plan.md             Host and hardware acceptance
```

## Build ESP-IDF firmware

Install and activate ESP-IDF using Espressif's official setup guide. Then:

```bash
git clone https://github.com/pardhuattili-web/ESP32-Modbus-RTU-Slave-Simulator-with-Fault-Injection.git
cd ESP32-Modbus-RTU-Slave-Simulator-with-Fault-Injection
idf.py set-target esp32
idf.py build
idf.py -p PORT flash monitor
```

Replace `PORT` with the board's serial port. The default UART0 console uses 115200 baud; Modbus is on UART2 using GPIO17/GPIO16.

## Run host tests

These tests do not require an ESP32 or the ESP-IDF toolchain:

```bash
cmake -S . -B build-host -DBUILD_HOST_TESTS=ON
cmake --build build-host
ctest --test-dir build-host --output-on-failure
python -m unittest discover -s tests -p 'test_*.py' -v
```

For physical serial tests, install pyserial:

```bash
python -m pip install pyserial
python tools/modbus_test_harness.py --port /dev/ttyUSB0 --baud 19200 --slave 1
```

On Windows use the appropriate COM port, for example `COM5`. The harness requires a USB-to-RS-485 adapter; a conventional USB-to-UART adapter is not the same interface.

## Register map

RTU register addresses are zero-based.

| Address | Bank | Meaning |
|---:|---|---|
| 0 | Holding | Setpoint, default 250 |
| 1 | Holding | Mode: 0 idle, 1 run, 2 fault |
| 2 | Holding | Fault mask: bit 0 drop, bit 1 delay, bit 2 corrupt CRC |
| 0 | Input | Simulated process value |
| 1 | Input | Firmware marker 0x0100 |
| 2 | Input | Uptime seconds (low word) |
| 3 | Input | Diagnostic flags |

Both banks contain 16 registers. Requests which cross the bank boundary are rejected.

## Important implementation notes

- RTU frame separation uses a 5 ms idle timeout for this demonstrator. It is not a fully baud-derived t1.5/t3.5 timing implementation.
- Broadcast requests are ignored in this simulator.
- The UART RX loop is educational and does not yet provide a hardened production path for UART overflow/framing errors or bus contention.
- Fault injection is designed for an isolated test bench. Do not use it on live controllers or safety-critical equipment.
- Hardware-dependent behavior such as transceiver turnaround and timing must be verified on your actual board.

## Roadmap

- [x] CRC-16/Modbus core and portable tests
- [x] FC03/04/06/16 handling and Modbus exceptions
- [x] Simulated register banks and one-shot fault hooks
- [x] Python serial smoke-test harness and unit tests
- [x] Host C/Python CI and engineering documentation
- [ ] Verify build on installed ESP-IDF version
- [ ] Verify USB/RS-485 physical communication and timing
- [ ] Capture real device logs and demonstration photos
- [ ] Add UART event-based RX framing and stronger overflow handling

## License

MIT; see [LICENSE](LICENSE).
