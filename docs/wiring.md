# Wiring Guide

## ESP32 to RS-485 transceiver

| ESP32 pin | Transceiver | Purpose |
|---|---|---|
| GPIO17 | DI | UART2 TX |
| GPIO16 | RO | UART2 RX |
| GPIO4 | DE and /RE tied together | HIGH=transmit, LOW=receive |
| GND | GND | Common signal reference |
| Appropriate supply | VCC | Follow module rating |
| A/B bus terminals | Twisted pair to USB-to-RS-485 adapter | Differential data |

**Important:** classic MAX485 breakouts may operate at 5 V and drive 5 V logic on RO. ESP32 GPIO is not 5 V tolerant. Use a 3.3 V-compatible transceiver or verified level translation.

## Bench bring-up
1. Build/flash with A/B disconnected.
2. Connect the adapter A/B per both devices' documentation. A/B labels vary between vendors.
3. Use 19200 baud, 8 data bits, no parity, 1 stop bit.
4. Run the test harness on the USB-to-RS-485 port.
5. Test fault injection only on an isolated lab setup, never on a live controller.