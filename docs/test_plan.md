# Test Plan

## Host tests

```bash
cmake -S . -B build-host -DBUILD_HOST_TESTS=ON
cmake --build build-host
ctest --test-dir build-host --output-on-failure
python -m unittest discover -s tests -p 'test_*.py' -v
```

Current automated coverage includes CRC known vector, register reads, invalid-address exceptions, single writes, bad CRC rejection, slave filtering, and dropped-response behavior.

## Hardware acceptance matrix

Do not mark these passed until tested with a physical ESP32 and RS-485 transceiver.

| Test | Procedure | Expected result |
|---|---|---|
| ESP-IDF build | `idf.py build` | Firmware image created |
| Boot | Flash and monitor | Startup reports address and UART config |
| FC03 | Read holding address 0, count 2 | Setpoint and mode returned |
| FC04 | Read input address 0, count 4 | Process value and diagnostics returned |
| FC06 | Write holding address 0 | Register changes and request echoed |
| Invalid address | Read address 15, count 2 | Exception 02 |
| Invalid CRC | Send bad CRC | No response; CRC counter increments |
| Drop fault | Set bit 0 in holding address 2 | Next response omitted |
| Delay fault | Set bit 1 | Next response delayed |
| CRC fault | Set bit 2 | Master reports invalid CRC |

Record ESP-IDF version, board, transceiver module, serial settings, and actual logs when hardware tests are complete.