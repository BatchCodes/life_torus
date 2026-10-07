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
- **Display modes.** Game of Life, scrolling text, rain, barber pole, ripples, sparkle and the music visualiser. Type the message in "Message to scroll" and press "Show message" or Enter, and set the speed of all modes. A mode stays on until you change it in the app, or until the power goes off. At power on, the display always starts with Game of Life. While another mode is on, the USB controller does nothing, except the ko code.
- **Presets.** Load any preset board directly.
- **Settings.** The step time, the unattended-play limits, the intensity, the brightness levels, the panel layout, beat sync and the app password. The display saves them in NVS, so they stay after a power cycle.

## Beat Sync

With the optional INMP441 microphone, the display modes move with the beat of music. Game of Life ignores the music. The [wiring guide](../hardware/power-and-wiring.md) shows how to connect the microphone.

- ripples: a new ring starts on each beat
- sparkle: a burst of sparkles on each beat
- rain: a row of drops starts on each beat
- barber pole: the stripes move one stripe per beat, and flash brighter on the beat
- scrolling text: the letters flash brighter on the beat
- visualiser: the bars flash brighter on the beat

## Music Visualiser

The visualiser shows the sound from the microphone as a spectrum around the ring: 32 bands from 40 Hz (bass) to 8 kHz (treble), each band 2 columns wide. Each band is a bar from the bottom. The top of a bar is bright, and a dim marker shows the recent peak. An automatic gain makes quiet and loud music both fill the display. With no microphone or no sound for 1 s, the visualiser plays a slow wave, so the display does not go dark.

The firmware listens in two bands at the same time: the bass (40 Hz to 150 Hz) and the full range (150 Hz to 6 kHz). It uses the bass beat when it is clear, and the full-range beat otherwise. Music from a phone speaker has almost no bass, so it uses the full range. For each band, it finds the tempo (60 to 180 BPM) and predicts the next beat, so the display is on the beat and not late. It reacts only when the tempo is steady for a few seconds. With no microphone, no music, or no steady beat, each mode keeps its normal timing. The "Tempo" line of the app shows the tempo: "about 120 BPM (estimating)" while the detector is still sure of nothing, and "120 BPM (locked, bass)" or "(locked, full range)" when the beat is steady and the modes follow it.

The settings page has two beat options: "Beat sync with the microphone" (on or off) and "Beat sensitivity" from 1 (only clear, loud beats) to 10 (quiet music too).

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
