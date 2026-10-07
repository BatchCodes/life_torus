# Flashing

This document tells you how to put the Life Torus firmware onto the ESP32-S3-DevKitC-1. There are three ways. Pick the first one that fits:

| Method        | You need                               | Your own firmware options |
| ------------- | -------------------------------------- | ------------------------- |
| web flasher   | Chrome or Edge                         | no, defaults only         |
| local ESP-IDF | Linux and the ESP-IDF install          | yes                       |
| Docker        | Docker (and esptool on macOS, Windows) | yes                       |

The firmware has two modes: the game, and the bring-up test patterns for a new display. Flash the bring-up firmware first. Refer to [Display Bring-Up](../hardware/bring-up.md). For all methods, connect the "UART" port of the ESP32-S3 board to your computer with a USB data cable. Some cables supply power only.

## Web Flasher

Open the [Life Torus web flasher](https://batchcodes.github.io/life_torus/flash.html) in Chrome or Edge. Click "Connect" under "Game" or "Bring-Up" and select the serial port of the board. The flasher erases the board and writes the firmware.

The web flasher writes the firmware with the default options. If your boards need other panel layout options, use one of the other methods.

If the board does not connect, hold the BOOT button, press and release the RST button, then release BOOT. Then try again.

## Local ESP-IDF

Install ESP-IDF once. Refer to the installation section in the [README](../../README.md). Put your options into `sdkconfig.local`. Refer to [Firmware Configuration](configuration.md). Then build and flash:

```bash
. ~/esp/esp-idf/export.sh
cd firmware/life_torus
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor
```

The monitor shows the log. Press `Ctrl+]` to stop it. To flash the bring-up firmware, add `CONFIG_LIFE_MODE_BRINGUP=y` to `sdkconfig.local`, delete `sdkconfig`, and build again.

## Docker

The Docker script runs `idf.py` in the official ESP-IDF image, at the pinned version. You do not need to install ESP-IDF.

On Linux, Docker can use the serial port, so the script builds and flashes:

```bash
./scripts/idf_docker.sh -p /dev/ttyUSB0 firmware/life_torus build flash monitor
```

On macOS and Windows, Docker cannot use the USB port. Build in Docker, then flash from your computer with esptool:

```bash
./scripts/idf_docker.sh firmware/life_torus build
python3 -m pip install esptool
cd firmware/life_torus/build
python3 -m esptool --chip esp32s3 write-flash @flash_args
```

## Release Images

Each [GitHub release](https://github.com/BatchCodes/life_torus/releases) has one merged image of the game and one of the bring-up firmware, with the default options. Write a merged image at offset 0x0:

```bash
python3 -m esptool --chip esp32s3 write-flash 0x0 life_torus-v0.1.0.bin
```

The release also has a `SHA256SUMS` file to check the download.

## See Also

- [Firmware Configuration](configuration.md): all options and defaults.
- [Display Bring-Up](../hardware/bring-up.md): test patterns and the checklist.
- [README](../../README.md): project overview.
