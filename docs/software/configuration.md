# Firmware Configuration

The Life Torus firmware has a set of options for the display hardware and the game. Each option has a default that works for the standard build, so you do not need to change anything. This document lists each option, its default and its effect.

The options are Kconfig options in [Kconfig.projbuild](../../firmware/life_torus/main/Kconfig.projbuild). The browser simulator has the same game options in its settings panel.

## Change an Option

Change options with `idf.py menuconfig` (menu "Life Torus"), or put them in a local file that git ignores:

```bash
cd firmware/life_torus
cp sdkconfig.local.example sdkconfig.local
# Edit sdkconfig.local, then build again with the new values:
rm -f sdkconfig
idf.py build
```

## Firmware Mode

| Option                     | Default | Effect                                                                                 |
| -------------------------- | ------- | -------------------------------------------------------------------------------------- |
| `CONFIG_LIFE_MODE_GAME`    | y       | The normal Game of Life display.                                                       |
| `CONFIG_LIFE_MODE_BRINGUP` | n       | Test patterns for a new display. Refer to [Display Bring-Up](../hardware/bring-up.md). |

## Display Hardware

| Option                              | Default | Effect                                                                                        |
| ----------------------------------- | ------- | --------------------------------------------------------------------------------------------- |
| `CONFIG_LIFE_PIN_DIN`               | 11      | GPIO for the data line of the first board.                                                    |
| `CONFIG_LIFE_PIN_CLK`               | 12      | GPIO for the clock line of all boards.                                                        |
| `CONFIG_LIFE_PIN_CS`                | 10      | GPIO for the CS (LOAD) line of all boards.                                                    |
| `CONFIG_LIFE_SPI_CLOCK_KHZ`         | 2000    | SPI clock. Reduce it if cells flicker at random.                                              |
| `CONFIG_LIFE_INTENSITY`             | 4       | Brightness of all LEDs, 0 to 15. With one 3 A USB-C input, keep it at 4 or less.              |
| `CONFIG_LIFE_BRIGHTNESS_LEVELS`     | y       | Born cells bright, surviving cells normal, dying cells dim. Turn off if the display flickers. |
| `CONFIG_LIFE_SUBFRAME_MS`           | 3       | Time of each of the 3 sub-frames for the brightness levels.                                   |
| `CONFIG_LIFE_REINIT_MS`             | 5000    | Period for a rewrite of the display start-up registers. 0: never.                             |
| `CONFIG_LIFE_PANEL_REVERSE_RING`    | n       | Panel layout: the chain goes right to left around the ring.                                   |
| `CONFIG_LIFE_PANEL_FLIP_BOARDS`     | n       | Panel layout: all boards upside down.                                                         |
| `CONFIG_LIFE_PANEL_ZIGZAG`          | n       | Panel layout: every second board turned by 180°.                                              |
| `CONFIG_LIFE_PANEL_BLOCK_TRANSPOSE` | n       | Panel layout: swap rows and columns in each 8 × 8 block.                                      |
| `CONFIG_LIFE_PANEL_BLOCK_FLIP_X`    | n       | Panel layout: mirror each block left to right.                                                |
| `CONFIG_LIFE_PANEL_BLOCK_FLIP_Y`    | n       | Panel layout: mirror each block top to bottom.                                                |

The bring-up firmware finds the panel layout options for your boards.

## Game

| Option                                | Default | Effect                                                                                |
| ------------------------------------- | ------- | ------------------------------------------------------------------------------------- |
| `CONFIG_LIFE_STEP_MS`                 | 100     | Time between two generations in run (10 generations per second).                      |
| `CONFIG_LIFE_EMPTY_LIMIT`             | 10      | Generations with an empty board before a new preset.                                  |
| `CONFIG_LIFE_NO_INPUT_LIMIT`          | 10000   | Generations with no button press before a new preset (approximately 17 minutes).      |
| `CONFIG_LIFE_REPEAT_LIMIT`            | 300     | Generations of a short repeat (period 32 or less) before a new preset. 0: off.        |
| `CONFIG_LIFE_PAUSE_TIMEOUT_MS`        | 30000   | Pause with no button press for this time changes to run.                              |
| `CONFIG_LIFE_TRANSITION_MS`           | 800     | Time of the wipe from the old board to a new preset.                                  |
| `CONFIG_LIFE_CURSOR_BLINK_MS`         | 250     | Half period of the cursor blink.                                                      |
| `CONFIG_LIFE_DPAD_REPEAT_DELAY_MS`    | 300     | A held D-pad button moves the cursor again after this time.                           |
| `CONFIG_LIFE_DPAD_REPEAT_INTERVAL_MS` | 80      | Time between moves while a D-pad button is held.                                      |
| `CONFIG_LIFE_KO_SCROLL_MS`            | 10000   | Ko code effect duration.                                                              |
| `CONFIG_LIFE_KO_GAP_MS`               | 2000    | Ko code button gap.                                                                   |
| `CONFIG_LIFE_EDGE_TORUS`              | y       | Edges of the empty and random presets: torus. The other presets have their own edges. |
| `CONFIG_LIFE_EDGE_CYLINDER`           | n       | Edges of the empty and random presets: cylinder (no wrap from the top to the bottom). |
| `CONFIG_LIFE_RANDOM_PERCENT`          | 30      | Live cells in the random preset.                                                      |
| `CONFIG_LIFE_LOG_CONTROLLER_REPORTS`  | n       | Log each raw controller report as hex.                                                |

## Unattended Play

The defaults keep the display interesting when nobody uses it, and after a person uses it and walks away:

- The display starts in run with a random preset.
- An empty board loads a new preset after 10 generations.
- A board that only repeats loads a new preset after 300 generations.
- 10,000 generations with no button press load a new preset.
- Pause with no button press for 30 s changes to run.

[Preset Boards](presets.md) shows how long each preset runs before one of these rules loads the next preset.

## See Also

- [Display Bring-Up](../hardware/bring-up.md): find the panel layout options and measure the current.
- [Preset Boards](presets.md): the presets and their run times.
