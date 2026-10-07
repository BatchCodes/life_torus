# Bill of Materials

This document lists the parts for one Life Torus: eight LED matrix boards in a ring, an ESP32-S3 controller board, a level shifter, USB-C power inputs and a USB game controller. A USB-C power bank or a USB-C wall charger powers the display. The [README](../../README.md) describes how the parts work together.

The suppliers and prices are examples from October 2026, in euros, with VAT. Equivalent parts from other suppliers work too, unless a note says otherwise.

## Main Parts

The default build has 8 boards: 64 columns around and 32 rows. You can use 1 to 16 boards. Each board adds 8 columns. Set the number of boards in the firmware options or in the phone app. Refer to [Firmware Configuration](../software/configuration.md). The lists below are for 8 boards.

| Part                    | Quantity    | Specification                                                                                                  | Example supplier                                                                                         | Approx. price |
| ----------------------- | ----------- | -------------------------------------------------------------------------------------------------------------- | -------------------------------------------------------------------------------------------------------- | ------------- |
| LED matrix board        | 8 (1 to 16) | Otronic LED Matrix 32x8 with MAX7219 (OT3522), red, 130 × 32.3 mm, 5 V                                         | [Otronic](https://www.otronic.nl/nl/led-matrix-32x8-met-max7219-module)                                  | € 6.25 each   |
| Controller board        | 1           | Espressif ESP32-S3-DevKitC-1-N8R8 (8 MB flash, 8 MB PSRAM)                                                     | [DigiKey](https://www.digikey.com/en/products/detail/espressif-systems/ESP32-S3-DEVKITC-1-N8R8/15295894) | € 16          |
| Level shifter           | 1           | 74AHCT125N (or 74HCT125N), quad buffer, DIP-14, with a DIP-14 socket                                           | any electronics supplier, for example Reichelt                                                           | € 1           |
| Power source            | 1 or 2      | USB-C power bank or USB-C wall charger, 5 V 3 A (15 W) or more per port, with a USB-C to USB-C cable rated 3 A | any electronics supplier                                                                                 | € 15 to € 30  |
| Game controller         | 1           | Rii USB classic controller (SNES layout, wired USB). Sold as a pack of 2.                                      | Amazon                                                                                                   | € 15 per pack |
| Microphone (optional)   | 1           | INMP441 I2S MEMS microphone breakout, 3.3 V, for beat sync of the display modes                                | any electronics supplier                                                                                 | € 3           |
| USB OTG adapter         | 1           | USB-A socket to the connector of your ESP32-S3 board's "USB" port (micro-B or USB-C)                           | any electronics supplier                                                                                 | € 3           |
| Mode button (optional)  | 1           | Momentary push button, normally open, 12 mm or 16 mm panel mount, for the display modes                        | any electronics supplier                                                                                 | € 1           |
| Power switch (optional) | 1           | Panel rocker or toggle switch, 1 pole, rated 10 A or more, in the 5 V bus                                      | any electronics supplier                                                                                 | € 2           |

## Power Wiring

| Part                   | Quantity | Specification                                                                                     | Approx. price |
| ---------------------- | -------- | ------------------------------------------------------------------------------------------------- | ------------- |
| USB-C power input      | 2        | USB-C socket breakout for power, with 5.1 kΩ resistors from CC1 and CC2 to GND, rated 3 A or more | € 2 each      |
| Schottky diode (power) | 2        | SB560 (5 A, 60 V, through-hole), one for each USB-C input                                         | € 0.40 each   |
| Main 5 V bus wire      | 2 m      | 1.0 mm² (18 AWG) stranded, one red and one black, from the USB-C inputs to the distribution       | € 3           |
| Branch wire            | 4 m      | 0.5 mm² (20 AWG) stranded, red and black, from the distribution to each pair of boards            | € 3           |
| Lever connectors       | 2        | 5-way lever connectors, for example WAGO 221-415, one for 5 V and one for GND                     | € 4           |
| Bulk capacitor         | 4        | 1000 µF, 10 V or higher, electrolytic, one at each power injection point                          | € 2           |

The USB-C input must have the two 5.1 kΩ resistors on the CC pins. Without them, a USB-C power bank or charger does not turn on its output. Do not use a USB-C "PD trigger" board that asks for 9 V, 12 V or 20 V: the LED boards and the ESP32-S3 board take 5 V only.

The second USB-C input is optional. With one input, the display gets up to 3 A, enough for the default brightness. A second input adds up to 3 A more, for a higher intensity. Refer to [Power and Wiring](power-and-wiring.md).

## Signal Wiring

| Part                 | Quantity | Specification                                                                    | Approx. price |
| -------------------- | -------- | -------------------------------------------------------------------------------- | ------------- |
| Prototype board      | 1        | Perforated board, approximately 50 × 70 mm, for the 74AHCT125 and its connectors | € 2           |
| Decoupling capacitor | 2        | 100 nF ceramic, one at the 74AHCT125 and one spare                               | € 0.20        |
| Schottky diode       | 1        | 1N5819, in the 5 V feed to the ESP32-S3 board                                    | € 0.20        |
| Jumper wires         | 40       | Female-to-female Dupont wires, 20 cm, for the board-to-board chain               | € 4           |
| Pin headers          | 1 strip  | 2.54 mm male headers, 40 pins                                                    | € 1           |

## Order List

Order these parts first. They have the longest delivery times, and you need them for the hardware tests:

1. 8 × Otronic LED Matrix 32x8 with MAX7219 (OT3522).
2. 1 × ESP32-S3-DevKitC-1-N8R8.
3. 2 × USB-C power input breakouts with 5.1 kΩ CC resistors, and 2 × SB560 diodes.
4. 1 × 74AHCT125N with a DIP-14 socket, and 2 × 100 nF capacitors.
5. 1 × USB OTG adapter for the ESP32-S3 board's "USB" port.

Then order the wire, the lever connectors, the bulk capacitors, the prototype board and the jumper wires. If you do not have a USB-C power bank or wall charger with a 3 A output, order one. The total cost of all parts, with the controller and without the power source, is approximately € 110.

## Notes

- The LED boards are red only. Life Torus uses brightness levels, not colours, to show the cell states.
- The total current of the eight boards depends on the brightness. The firmware limits the brightness by default, so one 3 A USB-C input is enough. Refer to [Power and Wiring](power-and-wiring.md).
- The ESP32-S3-DevKitC-1 does not always supply 5 V to a device on its "USB" port. If the controller does not start, connect it through a powered USB OTG adapter or a USB OTG Y-cable from the 5 V bus. The hardware tests confirm which method works.
- You do not need the "UART" port of the ESP32-S3 board for normal use. Use it to flash the firmware and to read the log.

## See Also

- [README](../../README.md): project overview.
- [Power and Wiring](power-and-wiring.md): signal chain, USB-C power and power distribution.
