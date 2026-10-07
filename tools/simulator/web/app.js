// SPDX-License-Identifier: GPL-3.0-or-later
//
// Draws the Life Torus display and sends the keyboard to the game code (compiled to WebAssembly).
"use strict";

const WIDTH = 64;
const HEIGHT = 32;
// Brightness levels 0 (off) to 3 (bright), as red LED shades.
const COLOURS = ["#1c0707", "#5e1410", "#c42a20", "#ff6a50"];
const MODE_NAMES = ["run", "pause", "new preset", "effect"];

// Same order as game::Button.
const KEYS = {
  ArrowUp: 0,
  ArrowDown: 1,
  ArrowLeft: 2,
  ArrowRight: 3,
  a: 4,
  b: 5,
  x: 6,
  y: 7,
  l: 8,
  r: 9,
  s: 10,
  " ": 11,
};

function keyButton(event) {
  const key = event.key.length === 1 ? event.key.toLowerCase() : event.key;
  return Object.prototype.hasOwnProperty.call(KEYS, key) ? KEYS[key] : undefined;
}

function drawFlat(ctx, levels) {
  const cw = ctx.canvas.width / WIDTH;
  const ch = ctx.canvas.height / HEIGHT;
  ctx.fillStyle = "#000";
  ctx.fillRect(0, 0, ctx.canvas.width, ctx.canvas.height);
  for (let y = 0; y < HEIGHT; ++y) {
    for (let x = 0; x < WIDTH; ++x) {
      ctx.fillStyle = COLOURS[levels[y * WIDTH + x]];
      ctx.beginPath();
      ctx.arc((x + 0.5) * cw, (y + 0.5) * ch, Math.min(cw, ch) * 0.4, 0, 2 * Math.PI);
      ctx.fill();
    }
  }
  // The board edges every 8 columns. The seam (column 63 to column 0) is at the left and right.
  ctx.strokeStyle = "#2a1c1c";
  ctx.lineWidth = 1;
  for (let x = 8; x < WIDTH; x += 8) {
    ctx.beginPath();
    ctx.moveTo(x * cw, 0);
    ctx.lineTo(x * cw, ctx.canvas.height);
    ctx.stroke();
  }
}

function drawRing(ctx, levels, angle) {
  const w = ctx.canvas.width;
  const h = ctx.canvas.height;
  ctx.fillStyle = "#000";
  ctx.fillRect(0, 0, w, h);
  // The real ring is 130 mm high and approximately 82 mm across (258 mm around).
  const ringHeight = h * 0.9;
  const radius = Math.min(w * 0.42, (ringHeight * 258) / 130 / (2 * Math.PI));
  const cx = w / 2;
  const top = (h - ringHeight) / 2;
  const rowHeight = ringHeight / HEIGHT;
  const columnWidth = (2 * Math.PI * radius) / WIDTH;
  // Back half first (dim, seen through the ring), then the front half.
  for (const front of [false, true]) {
    for (let x = 0; x < WIDTH; ++x) {
      const theta = (2 * Math.PI * (x + 0.5)) / WIDTH - angle;
      const depth = Math.cos(theta);
      if (front !== depth > 0) {
        continue;
      }
      const sx = cx + radius * Math.sin(theta);
      const cellWidth = Math.max(0.6, columnWidth * Math.abs(depth) * 0.8);
      ctx.globalAlpha = front ? 1 : 0.12;
      for (let y = 0; y < HEIGHT; ++y) {
        ctx.fillStyle = COLOURS[levels[y * WIDTH + x]];
        ctx.fillRect(
          sx - cellWidth / 2,
          top + y * rowHeight + rowHeight * 0.1,
          cellWidth,
          rowHeight * 0.8,
        );
      }
    }
  }
  ctx.globalAlpha = 1;
}

function readSettings(form) {
  const n = (name) => Number(form.elements[name].value);
  return {
    step_ms: n("step_ms"),
    brightness_levels: form.elements.brightness_levels.checked ? 1 : 0,
    settled_limit: n("settled_limit"),
    no_input_limit: n("no_input_limit"),
    repeat_limit: n("repeat_limit"),
    pause_timeout_ms: n("pause_timeout_ms"),
    ko_effect_ms: n("ko_effect_ms"),
    cylinder: form.elements.cylinder.checked ? 1 : 0,
    random_percent: n("random_percent"),
  };
}

async function main() {
  const module = await createLifeTorus();
  const api = {
    configure: module.cwrap("sim_configure", null, Array(10).fill("number")),
    start: module.cwrap("sim_start", null, ["number"]),
    press: module.cwrap("sim_press", null, ["number", "number"]),
    release: module.cwrap("sim_release", null, ["number", "number"]),
    tick: module.cwrap("sim_tick", null, ["number"]),
    render: module.cwrap("sim_render", "number", ["number"]),
    mode: module.cwrap("sim_mode", "number", []),
    generation: module.cwrap("sim_generation", "number", []),
    population: module.cwrap("sim_population", "number", []),
    cylinder: module.cwrap("sim_cylinder", "number", []),
    shapeName: module.cwrap("sim_shape_name", "string", []),
    presetName: module.cwrap("sim_preset_name", "string", []),
    cursorX: module.cwrap("sim_cursor_x", "number", []),
    setMode: module.cwrap("sim_set_mode", null, ["number", "number"]),
    setText: module.cwrap("sim_set_text", null, ["string"]),
    setSpeed: module.cwrap("sim_set_speed", null, ["number"]),
    bpm: module.cwrap("sim_bpm", "number", []),
    nextMode: module.cwrap("sim_next_mode", null, ["number"]),
    displayMode: module.cwrap("sim_display_mode", "number", []),
    listening: module.cwrap("sim_listening", "number", []),
    audioLevel: module.cwrap("sim_audio_level", "number", []),
  };

  const flat = document.getElementById("flat").getContext("2d");
  const ringCanvas = document.getElementById("ring");
  const ring = ringCanvas.getContext("2d");
  const showRing = document.getElementById("show-ring");
  const form = document.getElementById("settings");
  const status = {
    state: document.getElementById("state"),
    preset: document.getElementById("preset"),
    edges: document.getElementById("edges"),
    generation: document.getElementById("generation"),
    population: document.getElementById("population"),
    shape: document.getElementById("shape"),
  };

  // The game uses a 32-bit millisecond clock.
  const now = () => Math.floor(performance.now()) >>> 0;

  function restart() {
    const s = readSettings(form);
    const seed = (Math.random() * 0xffffffff) >>> 0 || 1;
    api.configure(
      s.step_ms,
      s.brightness_levels,
      s.settled_limit,
      s.no_input_limit,
      s.repeat_limit,
      s.pause_timeout_ms,
      s.ko_effect_ms,
      s.cylinder,
      s.random_percent,
      seed,
    );
    api.start(now());
  }

  const modes = document.getElementById("modes");
  modes.elements.mode.addEventListener("change", () => {
    api.setMode(Number(modes.elements.mode.value), now());
    // Give the keyboard back to the game.
    modes.elements.mode.blur();
  });
  modes.elements.text.addEventListener("input", () => api.setText(modes.elements.text.value));
  // Show the message: set it and change to the scrolling text mode.
  const showMessage = () => {
    api.setText(modes.elements.text.value);
    api.setMode(1, now());
    modes.elements.mode.value = "1";
    modes.elements.text.blur();
  };
  document.getElementById("show-message").addEventListener("click", showMessage);
  modes.elements.text.addEventListener("keydown", (event) => {
    if (event.key === "Enter") {
      event.preventDefault();
      showMessage();
    }
  });
  modes.elements.speed.addEventListener("input", () =>
    api.setSpeed(Number(modes.elements.speed.value)),
  );
  modes.elements.speed.addEventListener("change", () => modes.elements.speed.blur());
  modes.addEventListener("submit", (event) => event.preventDefault());
  const beatLine = document.getElementById("beat");
  const levelBar = document.getElementById("level");
  const listenButton = document.getElementById("listen");
  listenButton.addEventListener("click", async () => {
    listenButton.disabled = true;
    beatLine.textContent = "Asking for the microphone…";
    try {
      await startListening(module);
      listenButton.textContent = "● Listening";
      listenButton.classList.add("listening");
      // Game of Life ignores music, so show the visualiser.
      if (api.displayMode() === 0) {
        api.setMode(6, now());
        modes.elements.mode.value = "6";
      }
    } catch (error) {
      beatLine.textContent = "No microphone: " + error.message;
      listenButton.disabled = false;
    }
    listenButton.blur();
  });

  form.addEventListener("submit", (event) => {
    event.preventDefault();
    restart();
  });
  showRing.addEventListener("change", () => {
    ringCanvas.style.display = showRing.checked ? "block" : "none";
  });

  const held = new Set();
  window.addEventListener("keydown", (event) => {
    // Let the keys through, except while a person types in a text or number field.
    if (event.ctrlKey || event.metaKey || event.altKey) {
      return;
    }
    const target = event.target;
    if (
      target instanceof HTMLInputElement &&
      ["text", "number", "password"].includes(target.type)
    ) {
      return;
    }
    if (event.key.toLowerCase() === "m" && !event.repeat) {
      // The mode button of the display.
      api.nextMode(now());
      modes.elements.mode.value = String(api.displayMode());
      return;
    }
    const button = keyButton(event);
    if (button === undefined) {
      return;
    }
    event.preventDefault();
    // The game repeats a held D-pad button itself, so ignore the keyboard repeat.
    if (event.repeat || held.has(button)) {
      return;
    }
    held.add(button);
    api.press(button, now());
  });
  window.addEventListener("keyup", (event) => {
    const button = keyButton(event);
    if (button === undefined || !held.has(button)) {
      return;
    }
    held.delete(button);
    api.release(button, now());
  });
  window.addEventListener("blur", () => {
    for (const button of held) {
      api.release(button, now());
    }
    held.clear();
  });

  let angle = 0;
  function frame() {
    const t = now();
    api.tick(t);
    const pointer = api.render(t);
    const levels = module.HEAPU8.subarray(pointer, pointer + WIDTH * HEIGHT);
    drawFlat(flat, levels);
    if (showRing.checked) {
      const mode = api.mode();
      if (mode === 1) {
        // In pause, turn the ring so that the cursor faces the viewer.
        const target = (2 * Math.PI * (api.cursorX() + 0.5)) / WIDTH;
        let delta = target - angle;
        delta = Math.atan2(Math.sin(delta), Math.cos(delta));
        angle += delta * 0.15;
      } else {
        angle += 0.004;
      }
      drawRing(ring, levels, angle);
    }
    status.state.textContent = MODE_NAMES[api.mode()] ?? "-";
    status.preset.textContent = api.presetName();
    status.edges.textContent = api.cylinder() ? "cylinder" : "torus";
    status.generation.textContent = String(api.generation());
    status.population.textContent = String(api.population());
    status.shape.textContent = api.shapeName();
    if (api.listening()) {
      const bpm = api.bpm();
      const level = api.audioLevel();
      levelBar.style.width = level + "%";
      beatLine.textContent =
        level < 5
          ? "Listening, but the sound is very quiet. Check the microphone."
          : bpm > 0
            ? `Listening: ${bpm} BPM. The display modes move with the beat.`
            : "Listening: no steady beat yet. Play music with a clear beat.";
    }
    requestAnimationFrame(frame);
  }

  restart();
  requestAnimationFrame(frame);
}

main();
