# Assembly Guide

This guide tells you how to build a Life Torus from the parts in the [Bill of Materials](../hardware/bill-of-materials.md): eight LED boards in a ring, the controller electronics and the power supply. Do the steps in this order. Test the electronics on the bench before you close the ring. [Power and Wiring](../hardware/power-and-wiring.md) has the full wiring details.

The finished ring of 8 boards is approximately 130 mm high and 78 mm across the flat sides. It has 64 cells around and 32 cells from the top to the bottom. You can build a ring of 3 to 16 boards (1 or 2 boards make a flat display). Each board is 32.3 mm wide, so the ring is approximately 32.3 mm × the number of boards around. Set the number of boards in the firmware before the first test.

## Tools

- soldering iron and solder
- wire stripper and side cutter
- multimeter, with a current range of 10 A
- small screwdriver for the lever connectors
- hot glue or double-sided foam tape

## Step 1: Flash the Firmware

Flash the firmware before you wire anything. Then you know that the board works.

1. Connect the "UART" port of the ESP32-S3 board to your computer.
2. Flash the bring-up firmware. Refer to [Display Bring-Up](../hardware/bring-up.md) for the command.
3. Look at the serial monitor. The log must show `bring-up firmware`.

## Step 2: Build the Level Shifter

1. Solder the DIP-14 socket and a 5-pin header onto the prototype board.
2. Wire the socket as in [Power and Wiring](../hardware/power-and-wiring.md): inputs 1A, 2A and 3A from the ESP32-S3, outputs 1Y, 2Y and 3Y to the header, and the output enables and 4A to GND.
3. Solder the 100 nF capacitor between pin 14 and pin 7.
4. Put the 74AHCT125 into the socket. The notch points to pins 1 and 14.
5. Connect `GPIO11`, `GPIO12`, `GPIO10` and `G` of the ESP32-S3 to the prototype board.

## Step 3: Wire the Power on the Bench

Do this step with no USB-C cable connected.

1. Solder 1.0 mm² wires to the 5 V (VBUS) and GND pads of each USB-C power input. Check that each breakout has the 5.1 kΩ resistors on CC1 and CC2.
2. Connect the 5 V wire of each input to an SB560 diode, with the stripe away from the input. Join the two diode stripes, and connect them through the power switch (optional) to the 5 V lever connector. With no power switch, connect them straight to the lever connector. Connect the GND wires to the GND lever connector.
3. Prepare four branches of 0.5 mm² wire, red and black, each approximately 20 cm long. Solder a 1000 µF capacitor across the far end of each branch. The stripe on the capacitor is the negative side.
4. Connect the `5V` pin of the ESP32-S3 board to the 5 V lever connector through the 1N5819 diode, and a `G` pin to the GND lever connector.
5. Connect the VCC and GND pins of the level shifter header to the lever connectors.
6. Connect a USB-C power bank or charger to input 1. Measure approximately 4.6 V between the lever connectors. Disconnect it, connect it to input 2 and measure again. Then disconnect the power.

## Step 4: Test the Boards Flat on the Bench

Put the eight boards in a row on the bench, all upright: IN header at the bottom, OUT header at the top.

1. Connect the level shifter header to the IN header of board 1: `DIN`, `CLK`, `CS`.
2. Connect the OUT header of each board to the IN header of the next board with jumper wires: `DOUT` to `DIN`, `CLK` to `CLK`, `CS` to `CS`.
3. Connect `VCC` and `GND` with jumper wires only from board 1 to 2, 3 to 4, 5 to 6 and 7 to 8.
4. Connect the four power branches to the IN headers of boards 1, 3, 5 and 7.
5. Connect the power bank or charger to input 1. The bring-up firmware starts with the chip walk.
6. Do the bring-up checklist in [Display Bring-Up](../hardware/bring-up.md): chain order, panel layout, current, brightness levels and controller.
7. The bring-up firmware saves the panel layout and the intensity on the board.

Do not continue until the checklist passes. A wiring fault is much easier to find with the boards flat on the bench.

## Step 5: Build the Frame

The boards make an octagonal prism. A simple frame is enough:

- Cut two regular polygons from 3 mm plywood, acrylic or card, with one side for each board (an octagon for 8 boards). Each side is 32.3 mm long, the width of one board.
- Cut a centre hole in the bottom octagon for the jumper wires and the power branches.
- Glue the boards to the edges of the two octagons with hot glue or foam tape, LEDs out. Keep board 1 to board 8 in chain order around the ring. Look from the outside: the chain goes left to right.
- Put the USB-C sockets, and the optional power switch and mode button, in the wall of the base, so a person can reach them from the outside. Connect the mode button from `GPIO7` to `G`.
- The jumper wires and the power branches run inside the ring. The ESP32-S3, the level shifter and the USB-C inputs go into a base under the ring. Put the USB-C sockets in the wall of the base, so you can connect a cable from the outside.

Leave space at the seam between board 8 and board 1, so that the ring stays a regular octagon. A later phase of the project adds a printable frame with a diffuser.

## Step 6: Flash the Game

1. Flash the game firmware. Do not erase the board, so the saved panel layout stays. Refer to [Flashing](../software/flashing.md).
2. Connect the power bank or charger. The display shows a wipe, then starts a random preset.
3. Connect the controller. Press any button to pause. Move the cursor with the D-pad and press A to toggle a cell.

## WS2812B Build

This section is for the WS2812B build: flexible RGB panels around a support tube. The [Bill of Materials](../hardware/bill-of-materials.md#ws2812b-build) lists the parts. [Power and Wiring](../hardware/power-and-wiring.md#ws2812b-panels) has the wiring. Do steps 1 to 3 of the default build first, with these differences:

- Step 1: flash the bring-up firmware for WS2812B. Refer to [Display Bring-Up](../hardware/bring-up.md#ws2812b-panels).
- Step 2: use all four buffers of the 74AHCT125. Connect `GPIO13` to 4A (pin 12), not GND to pin 12. Solder a 330 Ω resistor in series with each output (1Y, 2Y, 3Y and 4Y), and a 6-pin header for the 4 data lines, 5 V and GND.
- Step 3: connect 2 or 3 USB-C inputs. Prepare one power branch for each panel, each with a 1000 µF capacitor at the far end.

### Size of the Ring

Each panel is approximately 80 mm wide, so the ring is 80 mm × the number of panels around. The panels are approximately 2 mm thick. Choose a support tube with an outer diameter of the ring diameter minus 4 mm.

| Panels | Columns | Around | Ring diameter        | Tube diameter        |
| ------ | ------- | ------ | -------------------- | -------------------- |
| 7      | 56      | 560 mm | approximately 178 mm | approximately 174 mm |
| 8      | 64      | 640 mm | approximately 204 mm | approximately 200 mm |
| 10     | 80      | 800 mm | approximately 255 mm | approximately 251 mm |
| 12     | 96      | 960 mm | approximately 306 mm | approximately 302 mm |

All rings are approximately 320 mm high. Seven panels make a ring of the size of a top hat.

The tube can be a cardboard tube, a plastic pipe, or a sheet of 0.5 mm to 1 mm polycarbonate or PETG. Roll the sheet into a tube of the correct diameter, and hold it with two rings or discs of plywood at the top and the bottom.

### Test the Panels Flat on the Bench

1. Put the panels in a row on the bench, all upright, in ring order from left to right. Put each panel with its data input at the same end.
2. Connect the data line outputs to `DIN` of the first panel of each line. Connect `DOUT` to `DIN` inside each line. Refer to Fig 5 in [Power and Wiring](../hardware/power-and-wiring.md#ws2812b-data-lines).
3. Connect a power branch to the power wires of each panel.
4. Connect two USB-C inputs. The bring-up firmware starts with the LED walk.
5. Do the WS2812B checklist in [Display Bring-Up](../hardware/bring-up.md#ws2812b-panels). The bring-up firmware saves the layout and the brightness on the board.

### Mount the Panels on the Tube

1. Cut a slot in the tube, or in the bottom disc, for the data wires and the power branches. All wires run inside the tube.
2. Fix panel 1 to the tube with double-sided tape or with cable ties through the holes of the panel, LEDs out. Keep the same end of each panel at the top as on the bench.
3. Fix the other panels in ring order. Look from the outside: the ring goes left to right. Push each panel against the previous panel, so the pixel pitch stays 10 mm across the joint.
4. Put the ESP32-S3, the level shifter and the USB-C inputs in a base under the tube, or inside the tube at the bottom. Put the USB-C sockets, the optional power switch and the optional mode button in a wall that a person can reach from the outside.
5. Flash the game firmware with the WS2812B display type. Do not erase the board, so the saved layout stays. Refer to [Flashing](../software/flashing.md).

A thin diffuser, for example a sheet of white paper or frosted film around the panels, makes the pixels softer.

## See Also

- [Bill of Materials](../hardware/bill-of-materials.md): parts, suppliers and prices.
- [Power and Wiring](../hardware/power-and-wiring.md): signal chain, USB-C power and power distribution.
- [Display Bring-Up](../hardware/bring-up.md): test patterns and the checklist.
