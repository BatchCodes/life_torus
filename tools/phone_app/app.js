// SPDX-License-Identifier: GPL-3.0-or-later
//
// The Life Torus phone app. On the display, it talks to the ESP32-S3 over a WebSocket. Anywhere
// else (for example on GitHub Pages), it runs the game itself in WebAssembly, so a person can
// try the app with no hardware. Messages: components/phone_link/include/phone_link/protocol.hpp.
"use strict";

let WIDTH = 64; // Columns: 8 per board. The display frames give the real width.
const HEIGHT = 32;
const COLOURS = ["#1c0707", "#5e1410", "#c42a20", "#ff6a50"];
const BUTTONS = ["up", "down", "left", "right", "a", "b", "x", "y", "l", "r", "select", "start"];
const MODES = [
  "Game of Life",
  "Scrolling text",
  "Rain",
  "Barber pole",
  "Ripples",
  "Sparkle",
  "Visualiser",
];
const GAME_STATES = ["run", "pause", "new preset", "effect"];
const SETTING_NUMBERS = [
  "boards",
  "step_ms",
  "settled_limit",
  "no_input_limit",
  "repeat_limit",
  "pause_timeout_ms",
  "random_percent",
  "intensity",
  "beat_sensitivity",
];
const SETTING_FLAGS = [
  "brightness_levels",
  "beat_sync",
  "cylinder",
  "reverse_ring",
  "flip_boards",
  "zigzag",
  "block_transpose",
  "block_flip_x",
  "block_flip_y",
];

// Packs 64 × 32 levels the same way as phone_link::pack_frame().
function packFrame(levels) {
  const out = new Uint8Array(3 + (WIDTH * HEIGHT) / 4);
  out[0] = 70; // 'F'
  out[1] = WIDTH;
  out[2] = HEIGHT;
  for (let i = 0; i < (WIDTH * HEIGHT) / 4; ++i) {
    let byte = 0;
    for (let k = 0; k < 4; ++k) {
      byte |= levels[i * 4 + k] << (k * 2);
    }
    out[3 + i] = byte;
  }
  return out;
}

// ----- Connection to the ESP32-S3.

class DeviceTransport {
  async login(password) {
    const response = await fetch("/login", {
      method: "POST",
      headers: { "Content-Type": "application/x-www-form-urlencoded" },
      body: "password=" + encodeURIComponent(password),
    });
    return response.ok;
  }

  connect(onFrame, onText, onState) {
    const socket = new WebSocket(`ws://${location.host}/ws`);
    socket.binaryType = "arraybuffer";
    socket.onopen = () => {
      onState("connected");
      socket.send("get_settings");
    };
    socket.onclose = () => {
      onState("disconnected");
      setTimeout(() => this.connect(onFrame, onText, onState), 2000);
    };
    socket.onmessage = (event) => {
      if (typeof event.data === "string") {
        onText(event.data);
      } else {
        onFrame(new Uint8Array(event.data));
      }
    };
    this.socket = socket;
  }

  send(text) {
    if (this.socket && this.socket.readyState === WebSocket.OPEN) {
      this.socket.send(text);
    }
  }
}

// ----- The same game in WebAssembly, for a try with no hardware.

class SimulatorTransport {
  async login(password) {
    return password === "life";
  }

  loadModule() {
    return new Promise((resolve, reject) => {
      const script = document.createElement("script");
      script.src = "../life_torus.js";
      script.onload = () => createLifeTorus().then(resolve, reject);
      script.onerror = reject;
      document.head.appendChild(script);
    });
  }

  async connect(onFrame, onText, onState) {
    const m = await this.loadModule();
    const c = (name, ret, args) => m.cwrap(name, ret, args);
    this.api = {
      configure: c("sim_configure", null, Array(10).fill("number")),
      start: c("sim_start", null, ["number"]),
      press: c("sim_press", null, ["number", "number"]),
      release: c("sim_release", null, ["number", "number"]),
      tick: c("sim_tick", null, ["number"]),
      render: c("sim_render", "number", ["number"]),
      mode: c("sim_mode", "number", []),
      generation: c("sim_generation", "number", []),
      population: c("sim_population", "number", []),
      shapeName: c("sim_shape_name", "string", []),
      presetName: c("sim_preset_name", "string", []),
      presetIndex: c("sim_preset_index", "number", []),
      presetCount: c("sim_preset_count", "number", []),
      presetNameAt: c("sim_preset_name_at", "string", ["number"]),
      setMode: c("sim_set_mode", null, ["number", "number"]),
      displayMode: c("sim_display_mode", "number", []),
      setText: c("sim_set_text", null, ["string"]),
      setSpeed: c("sim_set_speed", null, ["number"]),
      tapCell: c("sim_tap_cell", null, ["number", "number", "number"]),
      width: c("sim_width", "number", []),
      setBoards: c("sim_set_boards", null, ["number", "number"]),
      bpm: c("sim_bpm", "number", []),
      bpmEstimate: c("sim_bpm_estimate", "number", []),
      beatSource: c("sim_beat_source", "number", []),
      listening: c("sim_listening", "number", []),
      audioLevel: c("sim_audio_level", "number", []),
      setBeat: c("sim_set_beat", null, ["number", "number"]),
      selectPreset: c("sim_select_preset", null, ["number", "number"]),
    };
    this.module = m;
    this.onText = onText;
    this.settings = {
      step_ms: 100,
      settled_limit: 30,
      no_input_limit: 10000,
      repeat_limit: 300,
      pause_timeout_ms: 30000,
      random_percent: 30,
      boards: 8,
      intensity: 4,
      brightness_levels: 1,
      beat_sync: 1,
      beat_sensitivity: 5,
      cylinder: 0,
      reverse_ring: 0,
      flip_boards: 0,
      zigzag: 0,
      block_transpose: 0,
      block_flip_x: 0,
      block_flip_y: 0,
    };
    this.text = "Life Torus";
    this.speed = 5;
    this.applySettings();
    this.api.start(this.now());
    onState("simulator");

    const names = [];
    for (let i = 0; i < this.api.presetCount(); ++i) {
      names.push(encodeURIComponent(this.api.presetNameAt(i)));
    }
    onText("presets " + names.join("&"));
    onText("settings " + new URLSearchParams(this.settings).toString());

    setInterval(() => {
      const t = this.now();
      this.api.tick(t);
      const pointer = this.api.render(t);
      onFrame(packFrame(this.module.HEAPU8.subarray(pointer, pointer + WIDTH * HEIGHT)));
      onText("status " + this.status());
    }, 50);
  }

  now() {
    return Math.floor(performance.now()) >>> 0;
  }

  applySettings() {
    const s = this.settings;
    this.api.configure(
      s.step_ms,
      s.brightness_levels,
      s.settled_limit,
      s.no_input_limit,
      s.repeat_limit,
      s.pause_timeout_ms,
      10000,
      s.cylinder,
      s.random_percent,
      (Math.random() * 0xffffffff) >>> 0 || 1,
    );
  }

  status() {
    return new URLSearchParams({
      game: GAME_STATES[this.api.mode()] ?? "",
      display_mode: this.api.displayMode(),
      generation: this.api.generation(),
      population: this.api.population(),
      preset: this.api.presetIndex(),
      preset_name: this.api.presetName(),
      shape: this.api.shapeName(),
      speed: this.speed,
      text: this.text,
      mic: this.api.listening(),
      bpm: this.api.bpm(),
      bpm_estimate: this.api.bpmEstimate(),
      source: ["none", "bass", "full"][this.api.beatSource()],
    }).toString();
  }

  send(message) {
    const [command, ...rest] = message.split(" ");
    const argument = rest.join(" ");
    const t = this.now();
    switch (command) {
      case "press":
        this.api.press(BUTTONS.indexOf(argument), t);
        break;
      case "release":
        this.api.release(BUTTONS.indexOf(argument), t);
        break;
      case "cell":
        this.api.tapCell(Number(rest[0]), Number(rest[1]), t);
        break;
      case "preset":
        this.api.selectPreset(Number(argument), t);
        break;
      case "mode":
        this.api.setMode(Number(argument), t);
        break;
      case "text":
        this.text = decodeURIComponent(argument);
        this.api.setText(this.text);
        break;
      case "speed":
        this.speed = Number(argument);
        this.api.setSpeed(this.speed);
        break;
      case "settings":
        for (const [key, value] of new URLSearchParams(argument)) {
          if (key !== "password" && key in this.settings) {
            this.settings[key] = Number(value);
          }
        }
        this.applySettings();
        this.api.setBeat(this.settings.beat_sync, this.settings.beat_sensitivity);
        if (this.api.width() !== this.settings.boards * 8) {
          this.api.setBoards(this.settings.boards, t);
          WIDTH = this.api.width();
        }
        this.onText("settings " + new URLSearchParams(this.settings).toString());
        break;
      case "get_settings":
        this.onText("settings " + new URLSearchParams(this.settings).toString());
        break;
    }
  }
}

// ----- The page.

function main() {
  const onDevice = !location.hostname.endsWith("github.io") && !location.search.includes("sim");
  const transport = onDevice ? new DeviceTransport() : new SimulatorTransport();

  const loginPanel = document.getElementById("login");
  const app = document.getElementById("app");
  const canvas = document.getElementById("display");
  const ctx = canvas.getContext("2d");
  const status = document.getElementById("status");
  const modeSelect = document.getElementById("mode");
  const presetSelect = document.getElementById("preset");
  const textInput = document.getElementById("text");
  const speedInput = document.getElementById("speed");
  const settingsForm = document.getElementById("settings");
  let state = "connecting";
  let presetsKnown = false;

  MODES.forEach((name, i) => modeSelect.add(new Option(name, String(i))));

  function drawFrame(data) {
    if (data[0] !== 70) {
      return;
    }
    // The frame gives the width, which follows the number of boards.
    WIDTH = data[1];
    if (canvas.width !== WIDTH * 10) {
      canvas.width = WIDTH * 10;
    }
    const cw = canvas.width / WIDTH;
    const ch = canvas.height / HEIGHT;
    ctx.fillStyle = "#000";
    ctx.fillRect(0, 0, canvas.width, canvas.height);
    for (let i = 0; i < WIDTH * HEIGHT; ++i) {
      const level = (data[3 + (i >> 2)] >> ((i & 3) * 2)) & 3;
      ctx.fillStyle = COLOURS[level];
      ctx.fillRect((i % WIDTH) * cw + 1, Math.floor(i / WIDTH) * ch + 1, cw - 2, ch - 2);
    }
  }

  const tempoLine = document.getElementById("tempo");
  function showTempo(s) {
    if (s.get("mic") !== "1") {
      tempoLine.hidden = true;
      return;
    }
    const bpm = Number(s.get("bpm"));
    const estimate = Number(s.get("bpm_estimate"));
    tempoLine.hidden = false;
    tempoLine.textContent =
      "Tempo: " +
      (bpm > 0
        ? `${bpm} BPM (locked, ${s.get("source") === "bass" ? "bass" : "full range"})`
        : estimate > 0
          ? `about ${estimate} BPM (estimating)`
          : "no beat yet");
  }

  function beatText(s) {
    if (s.get("mic") !== "1") {
      return "";
    }
    const bpm = Number(s.get("bpm"));
    return bpm > 0 ? ` · ${bpm} BPM` : " · listening";
  }

  function onText(message) {
    const space = message.indexOf(" ");
    const kind = space < 0 ? message : message.slice(0, space);
    const body = space < 0 ? "" : message.slice(space + 1);
    if (kind === "status") {
      const s = new URLSearchParams(body);
      const mode = Number(s.get("display_mode"));
      showTempo(s);
      if (document.activeElement !== modeSelect) {
        modeSelect.value = String(mode);
      }
      if (document.activeElement !== presetSelect && presetsKnown) {
        presetSelect.value = s.get("preset");
      }
      if (document.activeElement !== speedInput) {
        speedInput.value = s.get("speed");
      }
      if (document.activeElement !== textInput && textInput.value === "") {
        textInput.value = s.get("text");
      }
      status.textContent =
        mode === 0
          ? `${s.get("game")} · ${s.get("preset_name")} · generation ${s.get("generation")} · ` +
            `${s.get("population")} cells · cursor: ${s.get("shape")}`
          : `${MODES[mode]}${state === "simulator" ? " · simulator" : ""}${beatText(s)}`;
    } else if (kind === "presets") {
      presetSelect.innerHTML = "";
      body
        .split("&")
        .forEach((name, i) => presetSelect.add(new Option(decodeURIComponent(name), String(i))));
      presetsKnown = true;
    } else if (kind === "settings") {
      const s = new URLSearchParams(body);
      for (const key of SETTING_NUMBERS) {
        if (s.has(key)) {
          settingsForm.elements[key].value = s.get(key);
        }
      }
      for (const key of SETTING_FLAGS) {
        if (s.has(key)) {
          settingsForm.elements[key].checked = s.get(key) === "1";
        }
      }
    }
  }

  function onState(newState) {
    state = newState;
    if (state === "disconnected") {
      status.textContent = "Connection lost. Trying again…";
    }
  }

  // Controller buttons: press on touch, release when the finger lifts.
  for (const button of document.querySelectorAll("[data-button]")) {
    const name = button.dataset.button;
    const down = (event) => {
      event.preventDefault();
      button.classList.add("active");
      transport.send("press " + name);
    };
    const up = (event) => {
      event.preventDefault();
      if (button.classList.contains("active")) {
        button.classList.remove("active");
        transport.send("release " + name);
      }
    };
    button.addEventListener("pointerdown", down);
    button.addEventListener("pointerup", up);
    button.addEventListener("pointerleave", up);
    button.addEventListener("pointercancel", up);
  }

  canvas.addEventListener("pointerdown", (event) => {
    const rect = canvas.getBoundingClientRect();
    const x = Math.floor(((event.clientX - rect.left) / rect.width) * WIDTH);
    const y = Math.floor(((event.clientY - rect.top) / rect.height) * HEIGHT);
    if (x >= 0 && x < WIDTH && y >= 0 && y < HEIGHT) {
      transport.send(`cell ${x} ${y}`);
    }
  });

  modeSelect.addEventListener("change", () => transport.send("mode " + modeSelect.value));
  presetSelect.addEventListener("change", () => transport.send("preset " + presetSelect.value));
  speedInput.addEventListener("change", () => transport.send("speed " + speedInput.value));
  textInput.addEventListener("change", () =>
    transport.send("text " + encodeURIComponent(textInput.value)),
  );
  // Show the message: send it and change to the scrolling text mode.
  const showMessage = () => {
    transport.send("text " + encodeURIComponent(textInput.value));
    transport.send("mode 1");
    modeSelect.value = "1";
    textInput.blur();
  };
  document.getElementById("show-message").addEventListener("click", showMessage);
  textInput.addEventListener("keydown", (event) => {
    if (event.key === "Enter") {
      event.preventDefault();
      showMessage();
    }
  });

  settingsForm.addEventListener("submit", (event) => {
    event.preventDefault();
    const values = new URLSearchParams();
    for (const key of SETTING_NUMBERS) {
      values.set(key, settingsForm.elements[key].value);
    }
    for (const key of SETTING_FLAGS) {
      values.set(key, settingsForm.elements[key].checked ? "1" : "0");
    }
    const password = settingsForm.elements.password.value;
    if (password !== "") {
      values.set("password", password);
      settingsForm.elements.password.value = "";
    }
    // The firmware reads %20 for a space, so replace the "+" of URLSearchParams.
    transport.send("settings " + values.toString().replace(/\+/g, "%20"));
    const saved = document.getElementById("saved");
    saved.hidden = false;
    setTimeout(() => (saved.hidden = true), 2000);
  });

  document.getElementById("login-form").addEventListener("submit", async (event) => {
    event.preventDefault();
    const ok = await transport.login(document.getElementById("password").value);
    document.getElementById("login-error").hidden = ok;
    if (!ok) {
      return;
    }
    loginPanel.hidden = true;
    app.hidden = false;
    transport.connect(drawFrame, onText, onState);
  });

  if (!onDevice) {
    document.getElementById("password").placeholder = "Simulator password: life";
    document.getElementById("listen-panel").hidden = false;
    const listen = document.getElementById("listen");
    const levelBar = document.getElementById("level");
    const listenStatus = document.getElementById("listen-status");
    listen.addEventListener("click", async () => {
      listen.disabled = true;
      listenStatus.textContent = "Asking for the microphone…";
      try {
        await startListening(transport.module);
        listen.textContent = "● Listening";
        listen.classList.add("listening");
        // Game of Life ignores music, so show the visualiser.
        if (modeSelect.value === "0") {
          transport.send("mode 6");
          modeSelect.value = "6";
        }
        setInterval(() => {
          const level = transport.api.audioLevel();
          const bpm = transport.api.bpm();
          levelBar.style.width = level + "%";
          listenStatus.textContent =
            level < 5
              ? "Listening, but the sound is very quiet."
              : bpm > 0
                ? `Listening: ${bpm} BPM.`
                : "Listening: no steady beat yet.";
        }, 100);
      } catch (error) {
        listenStatus.textContent = "No microphone: " + error.message;
        listen.disabled = false;
      }
    });
  }
}

main();
