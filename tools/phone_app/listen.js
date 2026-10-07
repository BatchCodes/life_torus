// SPDX-License-Identifier: GPL-3.0-or-later
//
// Sends the computer or phone microphone to the beat detector in the WebAssembly module, for
// beat sync of the display modes. Browsers allow the microphone on HTTPS and localhost only.
"use strict";

async function startListening(module) {
  const listen = module.cwrap("sim_listen", null, ["number"]);
  const bufferPointer = module.cwrap("sim_audio_buffer", "number", [])();
  const process = module.cwrap("sim_audio_process", null, ["number", "number"]);

  const stream = await navigator.mediaDevices.getUserMedia({
    audio: { echoCancellation: false, noiseSuppression: false, autoGainControl: false },
  });
  const context = new AudioContext();
  listen(context.sampleRate);
  const source = context.createMediaStreamSource(stream);
  // ScriptProcessorNode is old, but it needs no extra worklet file and works everywhere.
  const node = context.createScriptProcessor(1024, 1, 1);
  node.onaudioprocess = (event) => {
    const samples = event.inputBuffer.getChannelData(0);
    module.HEAPF32.set(samples, bufferPointer / 4);
    process(samples.length, Math.floor(performance.now()) >>> 0);
  };
  source.connect(node);
  // A node must connect to the output to run. The gain of 0 keeps the speaker silent.
  const mute = context.createGain();
  mute.gain.value = 0;
  node.connect(mute);
  mute.connect(context.destination);
  return context;
}
