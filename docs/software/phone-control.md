# Phone Control

A Life Torus can be controlled from a phone, in addition to the USB controller. The ESP32-S3 makes its own open Wi-Fi network. A phone joins it and opens the phone app in its browser. You do not install an app, and it works on iPhone and Android. The [phone app README](../../tools/phone_app/README.md) describes the app itself.

The display does not need a phone or a controller. With neither, it runs Game of Life by itself, with the unattended-play rules.

## Connect a Phone

1. Power on the Life Torus.
2. On the phone, join the Wi-Fi network `LIFE-TORUS`. It has no Wi-Fi password.
3. The phone opens the app in a sign-in window. If it does not, open `http://192.168.4.1` in the browser.
4. Enter the app password. The default password is `life`.

Up to four phones can connect at the same time. Each phone sees the live display and can use all controls.

## What the App Can Do

- **Live display.** A copy of the 64 × 32 display, approximately 10 frames per second. Tap a cell to toggle it. In run, a tap pauses the game first.
- **Controller.** The same buttons as the USB controller.
- **Display modes.** Game of Life, scrolling text, rain, barber pole, ripples and sparkle. Type the message for the scrolling text, and set the speed of all modes. A mode stays on until you change it in the app, or until the power goes off. At power on, the display always starts with Game of Life. While another mode is on, the USB controller does nothing, except the ko code.
- **Presets.** Load any preset board directly.
- **Settings.** The step time, the unattended-play limits, the intensity, the brightness levels, the panel layout and the app password. The display saves them in NVS, so they stay after a power cycle.

## Password

The app password stops casual visitors from changing the display. Set your own password in one of two ways:

- In the app: open "Settings", enter a new password and press "Save". Every phone must then log in again.
- In the firmware: put the password in `firmware/life_torus/sdkconfig.local`, which git ignores, and build the firmware:

```text
CONFIG_LIFE_WEB_PASSWORD="your password"
```

A password saved in the app replaces the firmware password. To go back to the firmware password, erase the board and flash it again.

**Limitation:** the Wi-Fi network is open and the app uses plain HTTP. A person with the right tools near the display can read the password. The password stops casual use only. It is not real security. Do not use a password that you use anywhere else.

## Wi-Fi Options

| Option                     | Default      | Effect                                          |
| -------------------------- | ------------ | ----------------------------------------------- |
| `CONFIG_LIFE_WIFI`         | y            | Make the Wi-Fi network and serve the phone app. |
| `CONFIG_LIFE_WIFI_SSID`    | `LIFE-TORUS` | The name of the Wi-Fi network.                  |
| `CONFIG_LIFE_WEB_PASSWORD` | `life`       | The default app password.                       |

Wi-Fi is always on when `CONFIG_LIFE_WIFI` is on. It uses approximately 100 mA more from the 5 V supply. Refer to [Power and Wiring](../hardware/power-and-wiring.md).

## How It Works

The `phone_link` component runs four parts:

- a Wi-Fi access point at `192.168.4.1`
- a small DNS server that answers every name with `192.168.4.1`, so the phone shows the app as a sign-in page (a captive portal)
- an HTTP server for the app files, which the firmware embeds, and for the login
- a WebSocket at `/ws` for the controls, the display frames and the status

The login gives the phone a session cookie. The WebSocket accepts only a phone with a valid session. The messages are in [protocol.hpp](../../components/phone_link/include/phone_link/protocol.hpp).

## Try the App with No Hardware

The [online phone app](https://batchcodes.github.io/life_torus/phone/) runs the same game in your browser, with the password `life`. Refer to the [phone app README](../../tools/phone_app/README.md).

## See Also

- [Phone App](../../tools/phone_app/README.md): the app files and the simulator version.
- [Firmware Configuration](configuration.md): all options and defaults.
