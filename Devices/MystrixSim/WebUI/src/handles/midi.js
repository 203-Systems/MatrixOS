/**
 * MIDI handle layer for programmatic send actions.
 *
 * This is the canonical write path for MIDI control operations. Keep JSON-RPC
 * and UI callers here so the control logic lives in one place.
 */

function getModule() {
  return window.Module ?? null
}

function sendMidi(status, data0, data1, data2) {
  const mod = getModule()
  if (!mod?._MatrixOS_Wasm_MidiSend) return false
  mod._MatrixOS_Wasm_MidiSend(status, data0 & 0xFF, data1 & 0xFF, data2 & 0xFF)
  return true
}

function sendMidiToPort(status, data0, data1, data2, targetPort) {
  const mod = getModule()
  if (!mod?._MatrixOS_Wasm_MidiSendToPort) return false
  mod._MatrixOS_Wasm_MidiSendToPort(status, data0 & 0xFF, data1 & 0xFF, data2 & 0xFF, targetPort)
  return true
}

export function sendMidiRaw(status, data0 = 0, data1 = 0, data2 = 0) {
  return sendMidi(status & 0xFF, data0, data1, data2)
}

export function sendMidiRawToPort(status, data0 = 0, data1 = 0, data2 = 0, targetPort = 0x0100) {
  return sendMidiToPort(status & 0xFF, data0, data1, data2, targetPort)
}

export function sendMidiSysEx(bytes) {
  const data = Array.from(bytes ?? []).map(byte => byte & 0xFF)
  if (data.length < 2 || data[0] !== 0xF0 || data[data.length - 1] !== 0xF7) {
    return false
  }

  const endFrameLength = ((data.length - 1) % 3) + 1
  const dataPacketLength = data.length - endFrameLength

  for (let index = 0; index < dataPacketLength; index += 3) {
    if (!sendMidiRaw(0xF0, data[index], data[index + 1], data[index + 2])) {
      return false
    }
  }

  const footer = [0, 0, 0]
  for (let index = 0; index < endFrameLength; index += 1) {
    footer[index] = data[dataPacketLength + index]
  }
  return sendMidiRaw(0xF7, footer[0], footer[1], footer[2])
}

export function sendMidiSysExToPort(bytes, targetPort = 0x0100) {
  const data = Array.from(bytes ?? []).map(byte => byte & 0xFF)
  if (data.length < 2 || data[0] !== 0xF0 || data[data.length - 1] !== 0xF7) {
    return false
  }

  const endFrameLength = ((data.length - 1) % 3) + 1
  const dataPacketLength = data.length - endFrameLength

  for (let index = 0; index < dataPacketLength; index += 3) {
    if (!sendMidiRawToPort(0xF0, data[index], data[index + 1], data[index + 2], targetPort)) {
      return false
    }
  }

  const footer = [0, 0, 0]
  for (let index = 0; index < endFrameLength; index += 1) {
    footer[index] = data[dataPacketLength + index]
  }
  return sendMidiRawToPort(0xF7, footer[0], footer[1], footer[2], targetPort)
}

export function sendMidiNote(channel, note, velocity) {
  const status = velocity > 0 ? (0x90 | (channel & 0x0F)) : (0x80 | (channel & 0x0F))
  return sendMidi(status, note, velocity, 0)
}

export function sendMidiNoteToPort(channel, note, velocity, targetPort) {
  const status = velocity > 0 ? (0x90 | (channel & 0x0F)) : (0x80 | (channel & 0x0F))
  return sendMidiToPort(status, note, velocity, 0, targetPort)
}

export function sendMidiCC(channel, controller, value) {
  const status = 0xB0 | (channel & 0x0F)
  return sendMidi(status, controller, value, 0)
}

export function sendMidiCCToPort(channel, controller, value, targetPort) {
  const status = 0xB0 | (channel & 0x0F)
  return sendMidiToPort(status, controller, value, 0, targetPort)
}

export function sendMidiProgramChange(channel, program) {
  const status = 0xC0 | (channel & 0x0F)
  return sendMidi(status, program, 0, 0)
}

export function sendMidiProgramChangeToPort(channel, program, targetPort) {
  const status = 0xC0 | (channel & 0x0F)
  return sendMidiToPort(status, program, 0, 0, targetPort)
}
