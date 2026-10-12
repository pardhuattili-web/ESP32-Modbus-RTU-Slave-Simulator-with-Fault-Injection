import importlib.util
import pathlib
import unittest

MODULE_PATH = pathlib.Path(__file__).parents[1] / "tools" / "modbus_test_harness.py"
SPEC = importlib.util.spec_from_file_location("modbus_test_harness", MODULE_PATH)
modbus = importlib.util.module_from_spec(SPEC)
assert SPEC and SPEC.loader
SPEC.loader.exec_module(modbus)


class ModbusHarnessTests(unittest.TestCase):
    def test_crc_known_vector(self):
        self.assertEqual(modbus.crc16_modbus(b"123456789"), 0x4B37)

    def test_crc_wire_order(self):
        payload = bytes([1, 3, 0, 0, 0, 1])
        frame = modbus.make_frame(payload)
        crc = modbus.crc16_modbus(payload)
        self.assertEqual(frame[-2:], bytes([crc & 0xFF, crc >> 8]))

    def test_empty_payload(self):
        self.assertEqual(modbus.make_frame(b""), b"\xff\xff")


if __name__ == "__main__":
    unittest.main()