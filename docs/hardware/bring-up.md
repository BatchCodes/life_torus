# Display Bring-Up

The bring-up firmware shows test patterns on a new display. Use it after you wire the boards for the first time. It checks the wiring and the chain order, finds the panel layout options for your boards, and measures the current. The [Bill of Materials](bill-of-materials.md) lists the parts.

The bring-up firmware is a mode of the normal firmware in `firmware/life_torus`. The game uses the panel layout options that you find here.

## Build and Flash the Bring-Up Firmware

Connect the "UART" port of the ESP32-S3 board to your computer. Build with the bring-up settings and flash:

```bash
. ~/esp/esp-idf/export.sh
cd firmware/life_torus
idf.py -B build_bringup -D SDKCONFIG=build_bringup/sdkconfig -D SDKCONFIG_DEFAULTS="sdkconfig.defaults;sdkconfig.ci.bringup" build
idf.py -B build_bringup -p /dev/ttyUSB0 flash monitor
```

The serial monitor shows the log. Each test pattern writes a line that tells you what to look for.

## Controls

Connect the USB controller to the "USB" port of the ESP32-S3 board, through the USB OTG adapter. The firmware logs each raw controller report as hex.

| Button | Function                                                |
| ------ | ------------------------------------------------------- |
| Start  | next test pattern                                       |
| Select | previous test pattern                                   |
| L / R  | lower / higher intensity (0 to 15)                      |
| X      | panel layout: swap rows and columns in each 8 × 8 block |
| Y      | panel layout: mirror each block left to right           |
| A      | panel layout: mirror each block top to bottom           |
| B      | panel layout: turn every second board by 180°           |
| Up     | panel layout: chain goes right to left around the ring  |
| Down   | panel layout: all boards upside down                    |

With no controller, the firmware shows each pattern for 8 seconds. The first button press stops the automatic change. After each layout change, the log shows the layout as `CONFIG_LIFE_PANEL_*` lines.

## Test Patterns

| Pattern      | What you see when everything is correct                                                      |
| ------------ | -------------------------------------------------------------------------------------------- |
| chip walk    | One 8 × 8 block at a time lights up, in chain order. Block 0 is the bottom block of board 1. |
| chip numbers | Each block shows its chain number, 0 to 31. The numbers are upright and readable.            |
| column sweep | One full column moves to the right around the ring, across all 32 rows.                      |
| row sweep    | One full row moves down, at the same height on all boards.                                   |
| all on       | All 2,048 LEDs are on. Use it to measure the current.                                        |
| levels       | The top band is bright, the middle band is normal and the bottom band is dim.                |

## Checklist

Do these checks in this order. Write down the results. They go into the documents and the firmware defaults.

1. **Chain order.** In the chip walk, the blocks must light up from the bottom of board 1 to the top, then board 2, and so on around the ring. If a block stays dark, check the jumper wires to that board. If the order is wrong, check which board gets `DOUT` from which.
2. **Panel layout.** Show the chip numbers. Press X, Y, A, B, Up and Down until all numbers are upright and readable, and increase from left to right around the ring. The firmware saves the layout and the intensity on the board after each change, and the game firmware uses them. If you build the firmware yourself, you can also copy the logged `CONFIG_LIFE_PANEL_*` lines into `firmware/life_torus/sdkconfig.local`. Then check the column sweep and the row sweep.
3. **Current.** Connect both USB-C inputs. Show all on. Measure the current in the 5 V wire from each USB-C input at intensities 0, 4 and 8, and add the two values. Do not go above intensity 8: the current can be more than two 3 A inputs can supply. The default intensity is 4.
4. **Power bank.** Disconnect input 2. Run the game firmware from one power bank on input 1 for 10 minutes. The display must not flicker, and the ESP32-S3 must not restart.
5. **Brightness levels.** Show levels. Look at the display directly and through a phone camera. If you see flicker or moving bands, change `CONFIG_LIFE_SUBFRAME_MS`, or turn off `CONFIG_LIFE_BRIGHTNESS_LEVELS`.
6. **Controller power.** Connect the controller to the "USB" port. The log must show `controller connected` and a report for each button. If the log shows nothing, the board does not supply 5 V to the port. Then use a powered USB OTG adapter or a USB OTG Y-cable from the 5 V supply.
7. **Controller layout.** Press each button and compare the logged report with the generic layout in [report.cpp](../../components/gamepad_input/src/report.cpp). The idle report is `01 7F 7F 7F 7F 0F 00 00`.

8. **Mode button and power switch.** Press the mode button. The log must show `mode button pressed` once for each press. Turn the power switch off and on. The display must go dark, then start again with the chip walk.
9. **Microphone.** Flash the game firmware. The log must show `microphone found`. Play music with a clear beat near the display, and open the phone app. After a few seconds, the status shows the tempo in BPM, and the display modes (for example ripples) move with the beat.

## WS2812B Panels

The bring-up firmware also works with WS2812B panels. Build it with the WS2812B display type, and set the number of panels with `CONFIG_LIFE_BOARDS` (`sdkconfig.ci.ws2812` sets 10):

```bash
. ~/esp/esp-idf/export.sh
cd firmware/life_torus
idf.py -B build_bringup_ws -D SDKCONFIG=build_bringup_ws/sdkconfig -D SDKCONFIG_DEFAULTS="sdkconfig.defaults;sdkconfig.ci.bringup;sdkconfig.ci.ws2812" build
idf.py -B build_bringup_ws -p /dev/ttyUSB0 flash monitor
```

A display type saved in the phone app replaces the firmware option.

### WS2812B Controls

| Button | Function                                                        |
| ------ | --------------------------------------------------------------- |
| Start  | next test pattern                                               |
| Select | previous test pattern                                           |
| L / R  | lower / higher brightness, in steps of 5 %                      |
| X      | layout: the LEDs run along the rows or along the columns        |
| Y      | layout: serpentine order (every second row runs back) on or off |
| A      | layout: the next start corner (the corner with the first LED)   |
| B      | layout: every second panel turned by 180°                       |
| Up     | layout: the panel order goes right to left around the ring      |

After each change, the log shows the layout and the estimated current of the last frame. The firmware saves the layout and the brightness on the board.

### WS2812B Test Patterns

| Pattern       | What you see when everything is correct                                                                                                      |
| ------------- | -------------------------------------------------------------------------------------------------------------------------------------------- |
| LED walk      | One lit LED moves along the LED order of each panel, on all panels at the same time. Line 1 is red, line 2 green, line 3 blue, line 4 white. |
| panel numbers | Each panel shows its number at the top, from 00, upright and readable. The bottom-left LED of each panel is white.                           |
| column sweep  | One full column moves to the right around the ring.                                                                                          |
| row sweep     | One full row moves down, at the same height on all panels.                                                                                   |
| all on        | All LEDs are white, at the current limit. Use it to measure the current.                                                                     |
| levels        | Three bands of rainbow colours: bright at the top, normal in the middle, dim at the bottom.                                                  |

### WS2812B Checklist

1. **Data lines.** In the LED walk, each panel must show one moving LED in the colour of its data line. If a panel stays dark, check its `DIN` wire and the `DOUT` wire of the previous panel. If a panel shows the wrong colour, it is on the wrong line.
2. **LED order.** Look at the start of the LED walk. Press A until the white corner LED of the panel numbers is at the bottom left. Press X and Y until the column sweep and the row sweep are straight lines.
3. **Panel order.** Show the panel numbers. They must increase from left to right around the ring. If they decrease, press Up. If every second panel is upside down, press B.
4. **Current.** Connect two USB-C inputs. Show all on. Measure the current in the 5 V wire from each input, and add the two values. The total must be near the current limit plus approximately 0.3 A. Press L and R and look at the logged estimate.
5. **Power bank.** Run the game firmware from your power source for 10 minutes. The display must not flicker, and the ESP32-S3 must not restart.

## Results

Record the results of your display here, and open an issue or a pull request with them.

| Check                              | Result         |
| ---------------------------------- | -------------- |
| panel layout options               | not yet tested |
| current at intensity 0 / 4 / 8     | not yet tested |
| one power bank, 10 minutes         | not yet tested |
| brightness levels                  | not yet tested |
| controller power on the "USB" port | not yet tested |
| controller report layout           | not yet tested |
| mode button and power switch       | not yet tested |
| microphone and beat sync           | not yet tested |
| WS2812B layout options             | not yet tested |
| WS2812B current at the limit       | not yet tested |

## See Also

- [Bill of Materials](bill-of-materials.md): parts, suppliers and prices.
- [README](../../README.md): project overview.
