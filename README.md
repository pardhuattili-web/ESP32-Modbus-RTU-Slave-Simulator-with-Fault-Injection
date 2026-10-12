# ESP32 Modbus RTU Slave Simulator with Fault Injection

![Platform](https://img.shields.io/badge/platform-ESP32-E7352C) ![Framework](https://img.shields.io/badge/framework-ESP--IDF-blue) ![Protocol](https://img.shields.io/badge/protocol-Modbus%20RTU-1976D2) ![Tests](https://img.shields.io/badge/tests-C%20%2B%20Python-informational)

A compact industrial-protocol test device for learning and demonstrating Modbus RTU over RS-485. The ESP32 firmware implements a small Modbus slave register map, supported function codes, protocol exceptions, diagnostics, and controlled fault-injection hooks for testing a Modbus master.

> **Status:** portable protocol core, ESP-IDF firmware skeleton, host unit tests, and engineering docs are included. Verify the ESP-IDF cross-build and physical serial behavior on an actual ESP32 + RS-485 transceiver before claiming hardware validation.

## Features
- Function codes 03 (Read Holding), 04 (Read Input), 06 (Write Single), and 16 (Write Multiple).
- CRC-16/Modbus checking and response generation (low CRC byte first).
- Register-range, quantity, request-length, and invalid-value validation.
- Modbus exception responses for unsupported functions, invalid addresses, and invalid values.
- UART2 half-duplex RS-485 direction control.
- One-shot response drop, delay, and CRC-corruption fault hooks.
- Diagnostics counters for received/valid frames, CRC errors, exceptions, and injected faults.
- Portable C protocol tests, Python serial smoke-test utility, documentation, and CI.

## Hardware
| Component | Quantity | Purpose |
|---|---:|---|
| ESP32 DevKit / ESP32-WROOM-32 | 1 | Slave firmware |
| 3.3 V logic-compatible RS-485 transceiver | 1 | Differential UART |
| USB-to-RS-485 adapter | 1 | PC/host master |
| USB data cable | 1 | Flashing and logs |
| Twisted pair | As needed | A/B bus |

**Electrical safety:** some MAX485 breakouts operate at 5 V and may output 5 V logic on RO. ESP32 GPIO is not 5 V tolerant. Use a 3.3 V-compatible transceiver or verified level shifting. Never connect A/B directly to ESP32 GPIO.

## Default pins
| ESP32 | Transceiver | Purpose |
|---|---|---|
| GPIO17 | DI | UART2 TX |
| GPIO16 | RO | UART2 RX |
| GPIO4 | DE + /RE tied | Direction control |
| GND | GND | Common reference |
| A/B | A/B to adapter | Differential bus |

Firmware defaults: UART2, 19200 baud, 8N1, slave address 1. A/B labels vary; follow both devices' manuals.

## Build ESP-IDF firmware
Install and activate ESP-IDF using Espressif's official setup guide, then:

```bash
git clone https://github.com/pardhuattili-web/ESP32-Modbus-RTU-Slave-Simulator-with-Fault-Injection.git
cd ESP32-Modbus-RTU-Slave-Simulator-with-Fault-Injection
idf.py set-target esp32
idf.py build
idf.py -p PORT flash monitor
```

Replace `PORT` with the board's serial port. UART0 console logs use 115200 baud; Modbus uses UART2 at 19200 baud.

## Run host tests
These tests need neither the ESP-IDF toolchain nor a physical board:

```bash
cmake -S . -B build-host -DBUILD_HOST_TESTS=ON
cmake --build build-host
ctest --test-dir build-host --output-on-failure
python -m unittest discover -s tests -p 'test_*.py' -v
```

For a physical test, install `pyserial` and use a USB-to-RS-485 adapter:

```bash
python -m pip install pyserial
python tools/modbus_test_harness.py --port /dev/ttyUSB0 --baud 19200 --slave 1
```

On Windows, replace the port with a COM port such as `COM5`.

## Register map
Requests use zero-based RTU register addresses.

| Address | Bank | Meaning |
|---:|---|---|
| 0 | Holding | Setpoint, default 250 |
| 1 | Holding | Mode: 0 idle, 1 run, 2 fault |
| 2 | Holding | Fault mask: bit 0 drop, bit 1 delay, bit 2 corrupt CRC |
| 0 | Input | Simulated process value |
| 1 | Input | Firmware marker 0x0100 |
| 2 | Input | Uptime seconds low word |
| 3 | Input | Diagnostic flags |

Each bank has 16 entries; operations crossing the implemented bounds are rejected.

## Engineering limitations
- The RTU receive loop uses a fixed 5 ms inter-frame idle timeout. It is a demonstrator, not a baud-derived t1.5/t3.5 implementation.
- The UART receive loop still needs production-strength overflow/framing-error recovery and timing validation.
- Broadcast requests are ignored in this simulator.
- Host tests do not validate UART timing, transceiver turnaround, electrical noise, or bus contention.
- Fault injection is intended for an isolated lab setup only, never a live control bus.

## Documentation
- [Architecture](docs/architecture.md)
- [Wiring guide](docs/wiring.md)
- [Protocol/register map](docs/protocol.md)
- [Test plan](docs/test_plan.md)

## Roadmap
- [x] CRC-16/Modbus core and portable tests
- [x] FC03/04/06/16 protocol handlers
- [x] Register map, exceptions, and fault hooks
- [x] Python smoke-test tool and host tests
- [x] CI and engineering docs
- [ ] Verify ESP-IDF compilation on the target toolchain
- [ ] Validate physical RS-485 timing and wiring
- [ ] Attach real board photos and serial logs
- [ ] Improve UART event handling and overflow recovery

## License
MIT — see [LICENSE](LICENSE).
