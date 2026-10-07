# Preset Boards

A preset is a full board that Life Torus can load. In pause, Select loads the next preset in the list below. When nobody uses the display, the firmware loads a preset at random, so the display stays interesting. The [README](../../README.md) describes the controls.

The pattern data comes from the [LifeWiki](https://conwaylife.com/wiki/). The presets are in [presets.cpp](../../components/patterns/src/presets.cpp), and the cursor shapes are in [library.cpp](../../components/patterns/src/library.cpp).

## Survey Results

The survey tool runs each preset on the 64 × 32 grid and finds when the board settles: the generation where it becomes empty or starts to repeat. "On display" is the number of generations until an unattended-play rule loads a new preset, with the default limits:

- empty for 10 generations
- a repeat with a period of 32 generations or less, for 300 generations
- 10,000 generations with no button press

At the default step time of 100 ms, 10 generations take 1 second.

| Preset            | Edges              | Cells | Settles at | End state           | On display |
| ----------------- | ------------------ | ----- | ---------- | ------------------- | ---------- |
| Glider gun        | torus              | 36    | 662        | period 2            | 964        |
| R-pentomino       | torus              | 5     | 1199       | period 2            | 1501       |
| Spaceship fleet   | torus              | 53    | 159        | period 2            | 461        |
| Acorn             | torus              | 7     | 340        | period 2            | 642        |
| Oscillator garden | torus              | 106   | 0          | period 30           | 330        |
| Glider swarm      | torus              | 40    | 293        | period 2            | 595        |
| Diehard           | torus              | 7     | 130        | empty               | 140        |
| Pi-heptomino      | torus              | 7     | 173        | period 2            | 475        |
| Simkin gun        | cylinder           | 36    | 469        | period 6            | 775        |
| Random soup       | configured (torus) | 632   | 3313       | period 2            | 3615       |
| Empty board       | configured (torus) | 0     | 0          | empty (for drawing) | 10         |

The random soup uses the random generator seed 1 and 30 % live cells. On the device the seed changes at each start, so its results are different each time. The empty board is for drawing in pause. The unattended-play rules never load it.

## Run the Survey

The tool needs a C++20 compiler and CMake. It does not need ESP-IDF:

```bash
cmake -S tools/preset_survey -B tools/preset_survey/build
cmake --build tools/preset_survey/build
tools/preset_survey/build/preset_survey
```

The tool prints the Markdown table above. When you change a preset, update this table and the expected values in [test_patterns.cpp](../../tests/host/main/test_patterns.cpp).

## Add a Preset

1. Add the shape to the cursor shape library in [library.cpp](../../components/patterns/src/library.cpp) if it is not there yet. Copy its RLE text from the LifeWiki.
2. Add a list of placements and a `Preset` entry in [presets.cpp](../../components/patterns/src/presets.cpp). Give each placement the top-left cell, the number of 90° turns and the mirror flag.
3. Run the survey. Keep the preset only if it stays interesting for a useful time.
4. Update the table in this document and the expected values in the host test.

## See Also

- [README](../../README.md): project overview and controls.
