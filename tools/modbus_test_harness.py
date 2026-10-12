#!/usr/bin/env python3
"""Small Modbus RTU smoke-test client (install pyserial for hardware use)."""
from __future__ import annotations

import argparse
import time


def crc16_modbus(data: bytes) -> int:
    crc = 0xFFFF
    for byte in data:
        crc ^= byte
        for _ in range(8):
            crc = (crc >> 1) ^ 0xA001 if crc & 1 else crc >> 1
    return crc & 0xFFFF


def make_frame(payload: bytes) -> bytes:
    crc = crc16_modbus(payload)
    return payload + bytes((crc & 0xFF, (crc >> 8) & 0xFF))


def read_registers(port, slave: int, function: int, start: int, count: int, timeout: float) -> bytes:
    payload = bytes((slave, function, start >> 8, start & 0xFF, count >> 8, count & 0xFF))
    port.reset_input_buffer()
    port.write(make_frame(payload))
    port.flush()
    deadline = time.monotonic() + timeout
    response = bytearray()
    while time.monotonic() < deadline:
        waiting = port.in_waiting
        if waiting:
            response.extend(port.read(waiting))
            if len(response) >= 5:
                expected = 5 + response[2] if response[1] == function else 5
                if len(response) >= expected:
                    break
        else:
            time.sleep(0.005)
    if len(response) < 5:
        raise TimeoutError(f"No complete response received; received={response.hex()}")
    packet = bytes(response)
    expected_crc = int.from_bytes(packet[-2:], "little")
    if crc16_modbus(packet[:-2]) != expected_crc:
        raise ValueError(f"Response CRC mismatch: {packet.hex()}")
    if packet[0] != slave:
        raise ValueError(f"Unexpected slave address {packet[0]}")
    if packet[1] & 0x80:
        raise RuntimeError(f"Modbus exception: function=0x{packet[1]:02X} code={packet[2]}")
    return packet


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--port", required=True, help="e.g. COM5 or /dev/ttyUSB0")
    parser.add_argument("--baud", type=int, default=19200)
    parser.add_argument("--slave", type=int, default=1)
    parser.add_argument("--timeout", type=float, default=1.0)
    parser.add_argument("--register", type=int, default=0)
    parser.add_argument("--count", type=int, default=3)
    args = parser.parse_args()

    try:
        import serial
    except ImportError:
        parser.error("pyserial is required: python -m pip install pyserial")

    if not 1 <= args.slave <= 247:
        parser.error("--slave must be in 1..247")
    if not 1 <= args.count <= 125:
        parser.error("--count must be in 1..125")
    if not 0 <= args.register <= 65535:
        parser.error("--register must be in 0..65535")

    with serial.Serial(args.port, args.baud, bytesize=8, parity="N", stopbits=1,
                       timeout=0.05, write_timeout=args.timeout) as port:
        for function, label in ((0x03, "holding"), (0x04, "input")):
            try:
                packet = read_registers(port, args.slave, function, args.register,
                                        args.count, args.timeout)
                values = [int.from_bytes(packet[i:i + 2], "big")
                          for i in range(3, 3 + packet[2], 2)]
                print(f"{label}: {values} raw={packet.hex()}")
            except (TimeoutError, ValueError, RuntimeError) as exc:
                print(f"{label} read failed: {exc}")
                return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
