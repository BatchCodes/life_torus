# Assembly Guide

This guide tells you how to build a Life Torus from the parts in the [Bill of Materials](../hardware/bill-of-materials.md): eight LED boards in a ring, the controller electronics and the power supply. Do the steps in this order. Test the electronics on the bench before you close the ring. [Power and Wiring](../hardware/power-and-wiring.md) has the full wiring details.

The finished ring is approximately 130 mm high and 78 mm across the flat sides. It has 64 cells around and 32 cells from the top to the bottom.

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
2. Connect the 5 V wire of each input through an SB560 diode to the 5 V lever connector, with the stripe towards the lever connector. Connect the GND wires to the GND lever connector.
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

- Cut two octagons from 3 mm plywood, acrylic or card, approximately 78 mm across the flat sides. Each side is 32.3 mm long, the width of one board.
- Cut a centre hole in the bottom octagon for the jumper wires and the power branches.
- Glue the boards to the edges of the two octagons with hot glue or foam tape, LEDs out. Keep board 1 to board 8 in chain order around the ring. Look from the outside: the chain goes left to right.
- The jumper wires and the power branches run inside the ring. The ESP32-S3, the level shifter and the USB-C inputs go into a base under the ring. Put the USB-C sockets in the wall of the base, so you can connect a cable from the outside.

Leave space at the seam between board 8 and board 1, so that the ring stays a regular octagon. A later phase of the project adds a printable frame with a diffuser.

## Step 6: Flash the Game

1. Flash the game firmware. Do not erase the board, so the saved panel layout stays. Refer to [Flashing](../software/flashing.md).
2. Connect the power bank or charger. The display shows a wipe, then starts a random preset.
3. Connect the controller. Press any button to pause. Move the cursor with the D-pad and press A to toggle a cell.

## See Also

- [Bill of Materials](../hardware/bill-of-materials.md): parts, suppliers and prices.
- [Power and Wiring](../hardware/power-and-wiring.md): signal chain, USB-C power and power distribution.
- [Display Bring-Up](../hardware/bring-up.md): test patterns and the checklist.
