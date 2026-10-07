# Phone App

The phone app controls a Life Torus from a phone browser. The ESP32-S3 makes an open Wi-Fi network called `LIFE-TORUS`. When a phone joins it, the phone opens this app. The app asks for the password, then shows a live copy of the display and the controls. No app installation is needed. [Phone Control](../../docs/software/phone-control.md) describes the system.

The app has:

- a live copy of the display: tap a cell to toggle it (in Game of Life, this pauses the game)
- the controller buttons, which work like the USB controller
- the display modes: Game of Life, scrolling text, rain, barber pole, ripples, sparkle and the music visualiser, with the message and the speed
- the preset list
- the settings: game limits, intensity, brightness levels, panel layout and the password

## Files

The firmware embeds `index.html`, `app.js` and `style.css` and serves them. The app sends the messages in [protocol.hpp](../../components/phone_link/include/phone_link/protocol.hpp) over a WebSocket.

## Try It with No Hardware

When the app does not run on a Life Torus (for example on GitHub Pages, or with `?sim` in the address), it runs the game itself in WebAssembly. The password is then `life`. Build the simulator and open the app:

```bash
./scripts/simulator_docker.sh --serve
```

Then open `http://localhost:8000/phone/?sim`. In the simulator, "Listen to music" uses the phone or computer microphone for beat sync and the visualiser, the same as a microphone on a real Life Torus. A level meter shows that it hears sound. The online version is at [batchcodes.github.io/life_torus/phone/](https://batchcodes.github.io/life_torus/phone/).

## See Also

- [Phone Control](../../docs/software/phone-control.md): Wi-Fi, login and settings.
- [Browser Simulator](../simulator/README.md): the simulator with the display ring.
