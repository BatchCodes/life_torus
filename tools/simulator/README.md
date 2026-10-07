# Browser Simulator

The browser simulator runs Life Torus on your computer, with no hardware. It compiles the same game code as the firmware (the `life`, `patterns`, `frame` and `game` components) to WebAssembly with Emscripten. A web page draws the 64 × 32 display and sends your keyboard to the game. Use it to try the game before you buy the parts, or to test a change to the game logic.

The page shows the display two times: as a ring, the way it stands on the table, and as a flat grid. The lines in the flat grid show the edges of the eight LED boards.

## Keys

| Key             | Controller button |
| --------------- | ----------------- |
| arrow keys      | D-pad             |
| `A` `B` `X` `Y` | A, B, X, Y        |
| `L` `R`         | L, R              |
| `S`             | Select            |
| Space           | Start             |
| `M`             | mode button       |

The [README](../../README.md) describes what each button does.

## Display Modes

The "Display mode" panel chooses what the display shows, the same as the phone app does on a real Life Torus: Game of Life, scrolling text, rain, barber pole, ripples, sparkle or the music visualiser. Type a message in "Message to scroll" and press "Show message" or Enter to scroll it, and the speed slider sets the speed of all modes. In a mode other than Game of Life, the keys do nothing, except the ko code.

## Beat Sync

"Listen to music" uses the microphone of your computer, through the browser, and runs the same beat detector and spectrum analyser as the firmware. The button turns red, and a level meter shows the sound that it hears. If the display shows Game of Life, it changes to the music visualiser. Play music with a clear bass beat. The "Tempo" field in the status shows the estimate after a few seconds ("estimating"), then "locked" when the beat is steady. Then the display modes (not Game of Life) move with the beat. The browser asks for permission first. It allows the microphone only on `https://` addresses and on `localhost`.

## Settings

The "Boards" field sets the number of boards, 1 to 16, like the firmware option. "Apply and restart" starts again with the new width. The "Display" field selects MAX7219 boards (red) or WS2812B panels (RGB). With WS2812B panels, the simulator shows the colours of the firmware, from the same `colour` component. "Single colour" and the colour picker show everything in one colour. The settings panel changes the same values as the firmware configuration, for example the step time and the unattended-play limits. Set a short limit, for example a no-input limit of 50 generations, to see a rule work in a few seconds. "Apply and restart" starts the game again with the new values and a new random seed.

## Build with Docker

The script builds the simulator in the official Emscripten image, at the version in [emsdk_version.txt](../../scripts/emsdk_version.txt). With `--serve`, it then serves the page on `http://localhost:8000`:

```bash
./scripts/simulator_docker.sh --serve
```

## Build with a Native emsdk

Install and load [emsdk](https://emscripten.org/docs/getting_started/downloads.html) at the pinned version, then build and serve the page:

```bash
./scripts/ci.sh simulator
python3 -m http.server 8000 --directory tools/simulator/web
```

Open `http://localhost:8000` in a browser. The build writes `life_torus.js` into `tools/simulator/web/`. Git ignores this file.

## See Also

- [README](../../README.md): project overview and controls.
- [Preset Boards](../../docs/software/presets.md): the presets that Select loads.
