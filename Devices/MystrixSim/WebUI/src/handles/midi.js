/**
 * MIDI handle layer for programmatic send actions.
 *
 * This is the canonical write path for MIDI control operations. Keep JSON-RPC
 * and UI callers here so the control logic lives in one place.
 */

function getModule() {
  return window.Module ?? null
}

function injectMidi(status, data0, data1, data2, targetPort) {
  const mod = getModule()
  if (!mod?._MatrixOS_Wasm_MidiInject) return false
  mod._MatrixOS_Wasm_MidiInject(status, data0 & 0x7F, data1 & 0x7F, data2 & 0x7F, targetPort)
  return true
}

export function sendMidiNote(channel, note, velocity, targetPort) {
  const status = velocity > 0 ? (0x90 | (channel & 0x0F)) : (0x80 | (channel & 0x0F))
  return injectMidi(status, note, velocity, 0, targetPort)
}

export function sendMidiCC(channel, controller, value, targetPort) {
  const status = 0xB0 | (channel & 0x0F)
  return injectMidi(status, controller, value, 0, targetPort)
}

export function sendMidiProgramChange(channel, program, targetPort) {
  const status = 0xC0 | (channel & 0x0F)
  return injectMidi(status, program, 0, 0, targetPort)
}
