# Contributing to Life Torus

Send changes as pull requests to [github.com/BatchCodes/life_torus](https://github.com/BatchCodes/life_torus). Report a problem or an idea as an issue. This document tells you how the repository is organised, how to build and test the project, and which code style to use. Install the tools first. Refer to the installation section in [README.md](README.md).

## Repository Layout

| Path             | Contents                                                                                 |
| ---------------- | ---------------------------------------------------------------------------------------- |
| `components/`    | Shared ESP-IDF components. The firmware apps, the host tests and the simulator use them. |
| `firmware/`      | One ESP-IDF app for each firmware image, for example `firmware/life_torus/`.             |
| `tests/host/`    | Host tests. They run on your computer with the ESP-IDF `linux` target.                   |
| `tools/`         | Computer tools, for example the browser simulator.                                       |
| `scripts/`       | Install, build and CI scripts, and the pinned ESP-IDF version.                           |
| `docs/hardware/` | Bill of materials, wiring, power and assembly.                                           |
| `docs/software/` | Configuration, presets and flashing.                                                     |

The shared components follow these rules:

- The pure components must not include ESP-IDF headers. The host tests and the browser simulator compile them on your computer.
- Hardware access is in separate components.
- Board-specific values (pins, SPI clock) live in the firmware app and its Kconfig. They do not live in the shared components.
- Timing and limit values are Kconfig options. They are not constants in the code. The pure components receive them in a configuration struct.

## Build

Load ESP-IDF, then build each app that your change touches:

```bash
. ~/esp/esp-idf/export.sh
cd firmware/life_torus
idf.py build
```

Or build in Docker, with no native installation:

```bash
./scripts/idf_docker.sh firmware/life_torus build
```

To change the ESP-IDF version, update [scripts/esp_idf_version.txt](scripts/esp_idf_version.txt), the image tag in [.devcontainer/devcontainer.json](.devcontainer/devcontainer.json) and the image tag in [.github/workflows/ci.yml](.github/workflows/ci.yml) in the same change.

## Test

The host tests use Unity on the ESP-IDF `linux` target. They test the pure C++ components on your computer, with no board. Run them before you send a change:

```bash
. ~/esp/esp-idf/export.sh
./scripts/run_host_tests.sh
```

The script builds `tests/host/` and runs the test binary. It exits with a non-zero status if a test fails. To add a test, add a `test_<name>.cpp` file in `tests/host/main/`, add it to `SRCS` in `tests/host/main/CMakeLists.txt`, and call its `run_<name>_tests()` function from `test_main.cpp`.

## Continuous Integration

GitHub Actions runs [scripts/ci.sh](scripts/ci.sh) on each push and pull request. To run the same checks locally in the same image:

```bash
docker run --rm -v "$PWD:/project" -w /project espressif/idf:v6.1 scripts/ci.sh all
```

The `simulator` command needs Emscripten, so `all` does not run it. CI builds the simulator in the `emscripten/emsdk` image. Locally, use [simulator_docker.sh](scripts/simulator_docker.sh). To change the Emscripten version, update [scripts/emsdk_version.txt](scripts/emsdk_version.txt) and the image tag in the workflow in the same change.

The container runs as `root`, so the build directories it makes belong to `root`. Delete them with `sudo`, or run the checks on a copy of the repository. `scripts/ci.sh` builds in `build_ci_*` directories, not in `build/`, so a CI run does not change the settings of your own `idf.py build`. CI does not read `sdkconfig.local`.

## Release

Push a version tag to make a release. The [release workflow](.github/workflows/release.yml) builds the game and the bring-up firmware, merges each into one image with `idf.py merge-bin`, and attaches the images and their SHA-256 checksums to a GitHub release:

```bash
git tag v0.1.0
git push origin v0.1.0
```

To build the same images locally, in `dist/`:

```bash
./scripts/ci.sh release v0.1.0
```

## GitHub Pages

The [Pages workflow](.github/workflows/pages.yml) runs on each push to `main`. It builds the browser simulator and the firmware images, and [build_site.sh](scripts/build_site.sh) puts them together in `site/`: the simulator as the main page, and the web flasher page from [tools/site](tools/site/flash.html). The site is at [batchcodes.github.io/life_torus](https://batchcodes.github.io/life_torus/).

## Code Style

- Format C and C++ code with `clang-format`. The project file is [.clang-format](.clang-format).
- Format Markdown, JSON and YAML with Prettier. The project file is [.prettierrc.json](.prettierrc.json).
- Check the spelling with cspell. Add a correct project word to [cspell.json](cspell.json).
- Use British English in documentation, for example "colour" and "licence".
- Write a commit message in the Conventional Commits format, for example `feat: add glider gun preset`.

Start each new source file with an SPDX licence header:

```cpp
// SPDX-License-Identifier: GPL-3.0-or-later
```

In a shell script, use `# SPDX-License-Identifier: GPL-3.0-or-later` on the line after the shebang.

## See Also

- [README.md](README.md): project overview, installation, build and flash.
- [LICENSE](LICENSE): GPL-3.0 licence text.
