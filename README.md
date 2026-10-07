# Life Torus

[![CI](https://github.com/BatchCodes/life_torus/actions/workflows/ci.yml/badge.svg)](https://github.com/BatchCodes/life_torus/actions/workflows/ci.yml)

Life Torus is a physical display for Conway's Game of Life. Eight red LED matrix boards stand in a ring and make a grid of 64 cells around and 32 cells high. An ESP32-S3 runs the simulation and drives the boards. A USB game controller moves a cursor, so you can draw cells and patterns on the ring. The grid wraps around the ring and from the top to the bottom, so the game runs on a torus.

This repository contains everything that you need to build one: the firmware, a bill of materials, the wiring and assembly documents, and three ways to flash the ESP32-S3. A [browser simulator](tools/simulator/README.md) lets you try the game before you buy the parts.

The source code is at [github.com/BatchCodes/life_torus](https://github.com/BatchCodes/life_torus). Report problems and ideas as GitHub issues.

> **Status: early development.** The repository skeleton is in place. The firmware, the documents and the simulator are not complete yet.

## How It Works

```text
USB game controller --USB--> ESP32-S3 --3.3 V SPI--> 74AHCT125 --5 V SPI--> board 1 --> board 2 --> ... --> board 8
                                 ^                                              ^
                                 |                                              |
                                 +-------------- 5 V power supply --------------+
```

- Each board is an 8 × 32 LED matrix with four MAX7219 driver chips. The eight boards make one chain of 32 chips.
- The ESP32-S3 sends the image over three wires (`DIN`, `CLK` and `CS`). A 74AHCT125 buffer changes the 3.3 V signals to 5 V.
- A 5 V power supply feeds the boards at several points. The ESP32-S3 uses the same supply.
- The game controller connects to the USB port of the ESP32-S3.

## Controls

The display has two states: run and pause. It starts in run.

| State | Button | Function                                               |
| ----- | ------ | ------------------------------------------------------ |
| run   | any    | pause                                                  |
| pause | Start  | run                                                    |
| pause | D-pad  | move the cursor                                        |
| pause | A      | stamp the cursor shape (single cell: toggle)           |
| pause | B      | erase the cells under the cursor shape                 |
| pause | L / R  | previous / next cursor shape                           |
| pause | X      | turn the cursor shape by 90°                           |
| pause | Y      | mirror the cursor shape                                |
| pause | Select | load the next [preset board](docs/software/presets.md) |

When nobody uses it, the display looks after itself. It loads a new preset when the board is empty, when the board only repeats, or after a long time with no button press. After 30 s in pause with no button press, it starts to run again.

## Supported Hardware

| Part                                                 | Status | Notes                                                    |
| ---------------------------------------------------- | ------ | -------------------------------------------------------- |
| Otronic LED Matrix 32x8 with MAX7219 (OT3522), 8 off | Target | The display boards.                                      |
| Espressif ESP32-S3-DevKitC-1-N8R8                    | Target | The controller board. It has a USB host port.            |
| Rii USB classic controller (SNES layout)             | Target | Other generic USB SNES controllers can work. Not tested. |

Refer to the [Bill of Materials](docs/hardware/bill-of-materials.md) for the full parts list, suppliers and prices.

## Required Tools

- A Linux computer. The install script supports Debian and Ubuntu. Other systems can install the tools manually or use Docker.
- The system packages in [system_packages.txt](system_packages.txt).
- ESP-IDF, the Espressif IoT Development Framework. The project pins the version in [scripts/esp_idf_version.txt](scripts/esp_idf_version.txt).
- Alternative to the native tools: Docker. Refer to [Build with Docker](#build-with-docker).

## Get the Code

```bash
git clone https://github.com/BatchCodes/life_torus.git
cd life_torus
```

Run all commands in this README from the repository root, unless a step says otherwise.

## Installation

The install script installs the system packages and the pinned ESP-IDF version in `~/esp/esp-idf`. It also adds your user to the `dialout` group for access to the serial port:

```bash
./scripts/install.sh
```

Load ESP-IDF in each new terminal:

```bash
. ~/esp/esp-idf/export.sh
```

## Run the Host Tests

The host tests build the pure C++ components on your computer and run them. You do not need a board:

```bash
./scripts/run_host_tests.sh
```

## Try the Simulator

The browser simulator runs the same game code on your computer. Build it in Docker and open `http://localhost:8000`:

```bash
./scripts/simulator_docker.sh --serve
```

Refer to [Browser Simulator](tools/simulator/README.md) for the keys and the native build.

## Build with Docker

If you do not want to install ESP-IDF, use the official ESP-IDF Docker image. The script uses the pinned version:

```bash
./scripts/idf_docker.sh firmware/life_torus build
```

On Linux, the script can also flash the board through the container:

```bash
./scripts/idf_docker.sh -p /dev/ttyACM0 firmware/life_torus flash monitor
```

On macOS and Windows, Docker cannot use the USB port. Build in Docker, then flash from your computer.

## Licence

Life Torus is free software under the GNU General Public License, version 3 or later. Refer to [LICENSE](LICENSE). The licence applies to the firmware, the scripts and the documents.

## See Also

- [Bill of Materials](docs/hardware/bill-of-materials.md): parts, suppliers and prices.
- [Browser Simulator](tools/simulator/README.md): try the game in a browser.
- [Preset Boards](docs/software/presets.md): the preset list and how long each preset runs.
- [CONTRIBUTING.md](CONTRIBUTING.md): repository layout, build, test and code style.
