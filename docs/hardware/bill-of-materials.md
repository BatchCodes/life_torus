# Bill of Materials

This document lists the parts for one Life Torus: eight LED matrix boards in a ring, an ESP32-S3 controller board, a level shifter, a 5 V power supply and a USB game controller. The [README](../../README.md) describes how the parts work together.

The suppliers and prices are examples from October 2026, in euros, with VAT. Equivalent parts from other suppliers work too, unless a note says otherwise.

## Main Parts

| Part             | Quantity | Specification                                                                        | Example supplier                                                                                         | Approx. price |
| ---------------- | -------- | ------------------------------------------------------------------------------------ | -------------------------------------------------------------------------------------------------------- | ------------- |
| LED matrix board | 8        | Otronic LED Matrix 32x8 with MAX7219 (OT3522), red, 130 × 32.3 mm, 5 V               | [Otronic](https://www.otronic.nl/nl/led-matrix-32x8-met-max7219-module)                                  | € 6.25 each   |
| Controller board | 1        | Espressif ESP32-S3-DevKitC-1-N8R8 (8 MB flash, 8 MB PSRAM)                           | [DigiKey](https://www.digikey.com/en/products/detail/espressif-systems/ESP32-S3-DEVKITC-1-N8R8/15295894) | € 16          |
| Level shifter    | 1        | 74AHCT125N (or 74HCT125N), quad buffer, DIP-14, with a DIP-14 socket                 | any electronics supplier, for example Reichelt                                                           | € 1           |
| Power supply     | 1        | Mean Well LRS-50-5, enclosed, 5 V 10 A (50 W), with over-current protection          | [OpenELAB](https://openelab.io/products/meanwell-lrs50-5-switching-power)                                | € 14          |
| Game controller  | 1        | Rii USB classic controller (SNES layout, wired USB). Sold as a pack of 2.            | Amazon                                                                                                   | € 15 per pack |
| USB OTG adapter  | 1        | USB-A socket to the connector of your ESP32-S3 board's "USB" port (micro-B or USB-C) | any electronics supplier                                                                                 | € 3           |

## Power Wiring

**Warning:** the power supply connects to mains voltage. Use an enclosed, certified supply. Do not leave a mains terminal open. If you do not have experience with mains wiring, ask a qualified person to do the mains side.

| Part                      | Quantity | Specification                                                                          | Approx. price |
| ------------------------- | -------- | -------------------------------------------------------------------------------------- | ------------- |
| Mains inlet               | 1        | IEC C14 inlet with a switch and a 5 × 20 mm fuse holder                                | € 4           |
| Mains fuse                | 2        | 5 × 20 mm, T2A (slow), one spare                                                       | € 1           |
| Mains cable               | 1        | IEC C13 cable for your country                                                         | € 4           |
| Main 5 V bus wire         | 2 m      | 1.0 mm² (18 AWG) stranded, one red and one black, from the supply to the distribution  | € 3           |
| Branch wire               | 4 m      | 0.5 mm² (20 AWG) stranded, red and black, from the distribution to each pair of boards | € 3           |
| Lever connectors          | 2        | 5-way lever connectors, for example WAGO 221-415, one for 5 V and one for GND          | € 4           |
| Bulk capacitor            | 4        | 1000 µF, 10 V or higher, electrolytic, one at each power injection point               | € 2           |
| Strain relief and grommet | 1 set    | For the mains cable and the 5 V wires                                                  | € 2           |

## Signal Wiring

| Part                 | Quantity | Specification                                                                    | Approx. price |
| -------------------- | -------- | -------------------------------------------------------------------------------- | ------------- |
| Prototype board      | 1        | Perforated board, approximately 50 × 70 mm, for the 74AHCT125 and its connectors | € 2           |
| Decoupling capacitor | 2        | 100 nF ceramic, one at the 74AHCT125 and one spare                               | € 0.20        |
| Jumper wires         | 40       | Female-to-female Dupont wires, 20 cm, for the board-to-board chain               | € 4           |
| Pin headers          | 1 strip  | 2.54 mm male headers, 40 pins                                                    | € 1           |

## Order List

Order these parts first. They have the longest delivery times, and you need them for the hardware tests:

1. 8 × Otronic LED Matrix 32x8 with MAX7219 (OT3522).
2. 1 × ESP32-S3-DevKitC-1-N8R8.
3. 1 × Mean Well LRS-50-5.
4. 1 × 74AHCT125N with a DIP-14 socket, and 2 × 100 nF capacitors.
5. 1 × USB OTG adapter for the ESP32-S3 board's "USB" port.

Then order the mains inlet, the fuses, the wire, the lever connectors, the bulk capacitors, the prototype board and the jumper wires. The total cost of all parts, with the controller, is approximately € 140.

## Notes

- The LED boards are red only. Life Torus uses brightness levels, not colours, to show the cell states.
- The total current of the eight boards depends on the brightness. The firmware limits the brightness by default. The power supply has a large margin.
- The ESP32-S3-DevKitC-1 does not always supply 5 V to a device on its "USB" port. If the controller does not start, connect it through a powered USB OTG adapter or a USB OTG Y-cable from the 5 V supply. The hardware tests confirm which method works.
- You do not need the "UART" port of the ESP32-S3 board for normal use. Use it to flash the firmware and to read the log.

## See Also

- [README](../../README.md): project overview.
