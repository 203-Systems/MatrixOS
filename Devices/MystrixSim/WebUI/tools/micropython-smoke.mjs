#!/usr/bin/env node
/**
 * MystrixSim MicroPython RPC smoke test.
 *
 * Requires a running WebUI dev server and an open MystrixSim browser tab:
 *   MATRIXOS_RPC_PORT=4012 VITE_MATRIXOS_RPC_PORT=4012 npm run dev -- --host 127.0.0.1 --port 5174
 *
 * Usage:
 *   node tools/micropython-smoke.mjs
 *   node tools/micropython-smoke.mjs --ws ws://localhost:4012
 *   node tools/micropython-smoke.mjs --suite ui --suite examples
 *   node tools/micropython-smoke.mjs --suite examples --example dice
 */

import { readFileSync } from 'node:fs'
import { dirname, join, resolve } from 'node:path'
import { fileURLToPath } from 'node:url'
import { WebSocket } from 'ws'

const DEFAULT_WS = 'ws://localhost:4002'
const TIMEOUT_MS = 15_000
const EXAMPLES = ['pixel_art', 'same_game', 'gomoku', 'dice']
const SUITE_NAMES = ['core', 'filesystem', 'ui', 'lifecycle', 'examples']

const toolDir = dirname(fileURLToPath(import.meta.url))
const repoRoot = resolve(toolDir, '..', '..', '..', '..')

let nextId = 1
const notifications = []

function parseArgs() {
  const args = process.argv.slice(2)
  const suites = []
  const examples = []
  let wsUrl = DEFAULT_WS

  for (let i = 0; i < args.length; i++) {
    const arg = args[i]
    if (arg === '--ws') {
      wsUrl = args[++i] || DEFAULT_WS
    } else if (arg === '--suite') {
      suites.push(...(args[++i] || '').split(',').map((item) => item.trim()).filter(Boolean))
    } else if (arg === '--example') {
      examples.push(...(args[++i] || '').split(',').map((item) => item.trim()).filter(Boolean))
    } else if (arg === '--help' || arg === '-h') {
      printUsage()
      process.exit(0)
    } else {
      throw new Error(`Unknown argument: ${arg}`)
    }
  }

  const selectedSuites = normalizeSuites(suites)
  const selectedExamples = normalizeExamples(examples)
  return {
    wsUrl,
    suites: selectedSuites,
    examples: selectedExamples,
  }
}

function printUsage() {
  console.log([
    'Usage:',
    '  node tools/micropython-smoke.mjs',
    '  node tools/micropython-smoke.mjs --ws ws://localhost:4012',
    '  node tools/micropython-smoke.mjs --suite core --suite ui',
    '  node tools/micropython-smoke.mjs --suite examples --example dice',
    '',
    'Suites:',
    '  all         Run every suite. Default.',
    '  core        Script launch, REPL, subscribe, input, USB, MIDI, HID, exceptions, API introspection.',
    '  filesystem  Multi-file staging, import, and open().',
    '  ui          Native UI wrapper interactions and callback safety.',
    '  lifecycle   Repeated start/stop, error recovery, and runtime responsiveness.',
    '  examples    Pixel Art, SameGame, Gomoku, and Dice interaction/startup checks.',
    '',
    'Examples:',
    `  ${EXAMPLES.join(', ')}`,
  ].join('\n'))
}

function normalizeSuites(rawSuites) {
  if (rawSuites.length === 0 || rawSuites.includes('all')) return SUITE_NAMES

  const unknown = rawSuites.filter((suite) => !SUITE_NAMES.includes(suite))
  if (unknown.length > 0) {
    throw new Error(`Unknown suite: ${unknown.join(', ')}`)
  }

  return Array.from(new Set(rawSuites))
}

function normalizeExamples(rawExamples) {
  if (rawExamples.length === 0 || rawExamples.includes('all')) return EXAMPLES

  const normalized = rawExamples.map((example) => example.replace(/\.py$/, ''))
  const unknown = normalized.filter((example) => !EXAMPLES.includes(example))
  if (unknown.length > 0) {
    throw new Error(`Unknown example: ${unknown.join(', ')}`)
  }

  return Array.from(new Set(normalized))
}

function fail(message) {
  throw new Error(message)
}

function assert(condition, message) {
  if (!condition) fail(message)
}

function connect(wsUrl) {
  return new Promise((resolve, reject) => {
    const socket = new WebSocket(wsUrl)
    const timer = setTimeout(() => reject(new Error('Connect timeout')), TIMEOUT_MS)

    socket.on('open', () => {
      clearTimeout(timer)
      socket.setMaxListeners(64)
      resolve(socket)
    })

    socket.on('error', (error) => {
      clearTimeout(timer)
      reject(error)
    })

    socket.on('message', (data) => {
      let payload
      try {
        payload = JSON.parse(data.toString())
      } catch {
        return
      }

      if (!payload.id && payload.method) {
        notifications.push(payload)
      }
    })
  })
}

function rpcCall(socket, method, params = {}, timeoutMs = TIMEOUT_MS) {
  return new Promise((resolve, reject) => {
    const id = `micropython-smoke-${nextId++}`
    const requestParams = {
      ...(params ?? {}),
      rpcTimeoutMs: Math.max(1000, timeoutMs - 500),
    }
    const timer = setTimeout(() => {
      socket.off('message', onMessage)
      reject(new Error(`Timeout waiting for ${method}`))
    }, timeoutMs)

    function onMessage(data) {
      let payload
      try {
        payload = JSON.parse(data.toString())
      } catch {
        return
      }

      if (payload?.type === 'connection_count' || payload?.id !== id) return

      clearTimeout(timer)
      socket.off('message', onMessage)

      if (payload.error) {
        reject(new Error(`${method}: ${JSON.stringify(payload.error)}`))
        return
      }

      resolve(payload.result)
    }

    socket.on('message', onMessage)
    socket.send(JSON.stringify({ jsonrpc: '2.0', id, method, params: requestParams }))
  })
}

function isPythonLaunchTimeout(error) {
  const message = String(error?.message ?? error)
  return (
    message.includes('Python script did not start') ||
    message.includes('Python app did not start') ||
    message.includes('Timeout waiting for python.runText') ||
    message.includes('Timeout waiting for python.runStaged')
  )
}

async function runPythonText(socket, params, timeoutMs = TIMEOUT_MS) {
  try {
    return await rpcCall(socket, 'python.runText', params, timeoutMs)
  } catch (error) {
    if (!isPythonLaunchTimeout(error)) throw error
    await stopPython(socket)
    return rpcCall(socket, 'python.runText', params, timeoutMs)
  }
}

async function runPythonStaged(socket, params = {}, timeoutMs = TIMEOUT_MS) {
  try {
    return await rpcCall(socket, 'python.runStaged', params, timeoutMs)
  } catch (error) {
    if (!isPythonLaunchTimeout(error)) throw error
    await stopPython(socket)
    return rpcCall(socket, 'python.runStaged', params, timeoutMs)
  }
}

async function waitForPythonActive(socket, expected, timeoutMs = 5_000) {
  const deadline = Date.now() + timeoutMs
  let status

  while (Date.now() < deadline) {
    status = await rpcCall(socket, 'python.status', {}, 2_000).catch(() => status)
    if (status?.active === expected) return status
    await new Promise((resolve) => setTimeout(resolve, 100))
  }

  return status
}

function outputText(output) {
  return (output.entries ?? []).map((entry) => entry.text).join('')
}

function summarizeLedFrame(frame) {
  if (!frame || frame.__error) return frame ?? null
  const grid = Array.isArray(frame.grid) ? frame.grid : []
  const underglow = Array.isArray(frame.underglow) ? frame.underglow : []
  return {
    timestamp: frame.timestamp,
    format: frame.format,
    gridLit: grid.filter((color) => color !== '00000000').length,
    underglowLit: underglow.filter((color) => color !== '00000000').length,
    gridPreview: grid.slice(0, 16),
    underglowPreview: underglow.slice(0, 16),
  }
}

async function diagnosticCall(socket, method, params = {}, timeoutMs = 2_000) {
  try {
    return await rpcCall(socket, method, params, timeoutMs)
  } catch (error) {
    return { error: String(error?.message ?? error) }
  }
}

async function collectDiagnostics(socket, context) {
  const [
    bridgeStatus,
    sessionStatus,
    runtimeState,
    appState,
    pythonStatus,
    pythonDebug,
    pythonOutput,
    activeInput,
    ledFrame,
    logs,
    emulatorErrors,
  ] = await Promise.all([
    diagnosticCall(socket, 'bridge.status'),
    diagnosticCall(socket, 'session.status'),
    diagnosticCall(socket, 'runtime.getState'),
    diagnosticCall(socket, 'runtime.getAppState'),
    diagnosticCall(socket, 'python.status'),
    diagnosticCall(socket, 'python.debug'),
    diagnosticCall(socket, 'python.getOutput', { last: 120 }),
    diagnosticCall(socket, 'input.get', { last: 120 }),
    diagnosticCall(socket, 'led.getFrame'),
    diagnosticCall(socket, 'log.get', { last: 120 }),
    diagnosticCall(socket, 'emulator.getErrors', { last: 40 }),
  ])

  return {
    context,
    bridge: bridgeStatus,
    session: sessionStatus,
    runtime: runtimeState,
    app: appState,
    python: pythonStatus,
    pythonDebug,
    activeInput,
    led: summarizeLedFrame(ledFrame),
    pythonOutput: pythonOutput?.entries ? outputText(pythonOutput).slice(-4000) : pythonOutput,
    logs,
    emulatorErrors,
  }
}

function formatDiagnostics(diagnostics) {
  return JSON.stringify(diagnostics, null, 2)
}

async function waitForOutput(socket, predicate, timeoutMs = 5_000) {
  const deadline = Date.now() + timeoutMs
  let output = ''

  while (Date.now() < deadline) {
    const response = await rpcCall(socket, 'python.getOutput', { last: 80 }, 5_000).catch(() => null)
    if (response) output = outputText(response)
    if (predicate(output)) return output
    await new Promise((resolve) => setTimeout(resolve, 100))
  }

  return output
}

async function stopPython(socket) {
  const deadline = Date.now() + 20_000
  let status = await rpcCall(socket, 'python.status').catch(() => null)

  while (Date.now() < deadline) {
    if (status?.active === false) return
    await rpcCall(socket, 'python.stop', { timeoutMs: 4_000 }, 5_000).catch(() => {})
    status = await waitForPythonActive(socket, false, 2_000)
    if (status?.active === false) return
  }

  const output = await rpcCall(socket, 'python.getOutput', { last: 80 }).catch(() => null)
  assert(
    status?.active === false,
    `Python app did not stop. Last status: ${JSON.stringify(status)} Output:\n${output ? outputText(output) : '<unavailable>'}`,
  )
}

async function clickInput(socket, input, holdMs = 120, settleMs = 80) {
  await rpcCall(socket, 'input.execute', {
    events: [
      { input, action: 'Press' },
      { input, action: 'Release', atMs: holdMs },
    ],
  })
  await new Promise((resolve) => setTimeout(resolve, holdMs + settleMs))
}

async function holdFunctionKeyToExitExample(socket, exampleName) {
  await rpcCall(socket, 'input.execute', {
    events: [
      { input: 'function', action: 'Press' },
      { input: 'function', action: 'Press', atMs: 700 },
      { input: 'function', action: 'Release', atMs: 900 },
    ],
  })
  const finalStatus = await waitForPythonActive(socket, false, 5_000)
  assert(finalStatus?.active === false, `${exampleName} did not exit on function-key hold`)
  await new Promise((resolve) => setTimeout(resolve, 300))
  await rpcCall(socket, 'input.releaseAll').catch(() => {})
  await new Promise((resolve) => setTimeout(resolve, 100))
}

async function smokeRunText(socket) {
  await stopPython(socket)

  const result = await runPythonText(socket, {
    name: 'smoke.py',
    text: "import MatrixOS\nprint('hello micro')\nprint(MatrixOS.LED.count())",
  }, 60_000)

  assert(result.ok, 'python.runText did not return ok')
  await waitForPythonActive(socket, false)

  const output = await waitForOutput(socket, (text) => (
    text.includes('hello micro') &&
    /\b\d+\b/.test(text)
  ), 8_000)
  assert(output.includes('hello micro'), 'python.runText output did not include hello marker')
  assert(/\b\d+\b/.test(output), 'python.runText output did not include LED count')

  console.log('[micropython-smoke] runText ok')
}

async function smokeRepl(socket) {
  await stopPython(socket)

  let repl
  try {
    repl = await rpcCall(socket, 'python.enterRepl', {}, 30_000)
  } catch (error) {
    await stopPython(socket)
    repl = await rpcCall(socket, 'python.enterRepl', {}, 30_000)
  }
  assert(repl.ok && repl.mode === 'repl', 'python.enterRepl failed')

  await rpcCall(socket, 'python.input', { text: "print('repl ok')\n" })

  await rpcCall(socket, 'python.input', {
    text: [
      'total = 0',
      'for i in range(3):',
      '    total += i',
      '',
      'print(total)',
      '',
    ].join('\n'),
  })

  const output = await waitForOutput(socket, (text) => (
    text.includes('Matrix OS MicroPython') &&
    text.includes('repl ok') &&
    text.includes('... ') &&
    /(?:^|[\r\n])3[\r\n]/.test(text)
  ), 8_000)
  assert(output.includes('Matrix OS MicroPython'), 'REPL banner missing')
  assert(output.includes('repl ok'), 'REPL print output missing')
  assert(output.includes('... '), 'REPL continuation prompt missing')
  assert(/(?:^|[\r\n])3[\r\n]/.test(output), 'REPL multiline block output missing')

  await stopPython(socket)
  console.log('[micropython-smoke] REPL ok')
}

async function smokeSubscribe(socket) {
  await stopPython(socket)
  notifications.length = 0

  const subscribed = await rpcCall(socket, 'python.subscribe')
  assert(subscribed.ok, 'python.subscribe failed')

  await runPythonText(socket, {
    name: 'subscribe_smoke.py',
    text: "print('sub hello')",
  })
  await waitForPythonActive(socket, false)
  await new Promise((resolve) => setTimeout(resolve, 300))

  assert(
    notifications.some((item) => item.method === 'python.event' && item.params?.payload?.includes('sub hello')),
    'python.subscribe did not receive output notification',
  )

  console.log('[micropython-smoke] subscribe ok')
}

async function smokeInput(socket) {
  await stopPython(socket)

  const result = await runPythonText(socket, {
    name: 'input_smoke.py',
    text: [
      'import MatrixOS',
      "print('ready')",
      'def loop():',
      '    e = MatrixOS.Input.get_event()',
      '    if e:',
      '        print(e)',
      '',
    ].join('\n'),
  })
  assert(result.ok, 'input smoke script did not start')

  const status = await waitForPythonActive(socket, true)
  assert(status?.active === true && status.mode === 'app', 'input smoke script is not active')

  await rpcCall(socket, 'input.execute', {
    events: [
      { input: 'grid:0,0', action: 'Press' },
      { input: 'grid:0,0', action: 'Release' },
      { input: 'touchbar:left:2', action: 'Press', atMs: 120 },
      { input: 'touchbar:left:2', action: 'Release', atMs: 240 },
    ],
  })

  const output = await waitForOutput(socket, (text) => text.includes("'released': True") && text.includes("'point': (-1, 2)"))
  assert(output.includes('ready'), 'input smoke script did not print ready')
  assert(output.includes("'pressed': True"), 'press input event missing')
  assert(output.includes("'released': True"), 'release input event missing')
  assert(output.includes("'point': (0, 0)"), 'input point mapping missing')
  assert(output.includes("'point': (-1, 2)"), 'touchbar input point mapping missing')

  await stopPython(socket)
  console.log('[micropython-smoke] input ok')
}

async function smokeUsbCdc(socket) {
  await stopPython(socket)

  const result = await runPythonText(socket, {
    name: 'usb_cdc_smoke.py',
    text: [
      'import MatrixOS',
      'USB = MatrixOS.USB',
      'SYS = MatrixOS.SYS',
      'ready = False',
      'done = False',
      '',
      'def loop():',
      '    global ready, done',
      '    if not ready:',
      '        ready = True',
      '        print("usb_cdc_ready", USB.connected(), USB.CDC.connected())',
      '    if done:',
      '        return',
      '    if USB.CDC.available() >= 5:',
      '        data = USB.CDC.read_bytes(5)',
      '        print("usb_cdc_data", data)',
      '        USB.CDC.print("tx")',
      '        USB.CDC.println("line")',
      '        USB.CDC.flush()',
      '        done = True',
      '        SYS.exit_app()',
      '',
    ].join('\n'),
  })
  assert(result.ok, 'USB CDC smoke script did not start')

  const status = await waitForPythonActive(socket, true)
  assert(status?.active === true && status.mode === 'app', 'USB CDC smoke script is not active')

  await waitForOutput(socket, (text) => text.includes('usb_cdc_ready'))
  const sent = await rpcCall(socket, 'serial.send', { payload: 'abcde' })
  assert(sent.ok, 'serial.send failed')

  const hasExpectedUsbCdcData = (text) => text.includes("usb_cdc_data b'abcde'")

  let output = await waitForOutput(socket, hasExpectedUsbCdcData, 3_000)
  if (!hasExpectedUsbCdcData(output)) {
    const retried = await rpcCall(socket, 'serial.send', { payload: 'abcde' })
    assert(retried.ok, 'serial.send retry failed')
    output = await waitForOutput(socket, hasExpectedUsbCdcData, 5_000)
  }
  assert(output.includes("b'abcde'"), 'USB CDC read_bytes did not receive injected serial data')

  const finalStatus = await waitForPythonActive(socket, false)
  assert(finalStatus?.active === false, 'USB CDC smoke script did not exit')

  console.log('[micropython-smoke] USB CDC ok')
}

async function smokeMidi(socket) {
  await stopPython(socket)

  const result = await runPythonText(socket, {
    name: 'midi_smoke.py',
    text: [
      'import MatrixOS',
      'MIDI = MatrixOS.MIDI',
      'SYS = MatrixOS.SYS',
      'ready = False',
      'done = False',
      '',
      'def loop():',
      '    global ready, done',
      '    if not ready:',
      '        ready = True',
      '        print("midi_ready")',
      '    if done:',
      '        return',
      '    packet = MIDI.get(0)',
      '    if packet is not None:',
      '        print("midi_data", packet.status(), packet.channel(), packet.note(), packet.velocity(), packet.port(), packet.data())',
      '        done = True',
      '        SYS.exit_app()',
      '',
    ].join('\n'),
  })
  assert(result.ok, 'MIDI smoke script did not start')

  const status = await waitForPythonActive(socket, true)
  assert(status?.active === true && status.mode === 'app', 'MIDI smoke script is not active')

  await waitForOutput(socket, (text) => text.includes('midi_ready'))
  const sent = await rpcCall(socket, 'midi.send', {
    targetPort: 0xF000,
    message: { kind: 'note_on', channel: 1, note: 64, velocity: 96 },
  })
  assert(sent.ok, 'midi.send failed')

  const hasExpectedMidiPacket = (text) => (
    text.includes('midi_data 144 0 64 96 256') &&
    text.includes('(144, 64, 96)')
  )

  let output = await waitForOutput(socket, hasExpectedMidiPacket, 3_000)
  if (!hasExpectedMidiPacket(output)) {
    const retried = await rpcCall(socket, 'midi.send', {
      targetPort: 0xF000,
      message: { kind: 'note_on', channel: 1, note: 64, velocity: 96 },
    })
    assert(retried.ok, 'midi.send retry failed')
    output = await waitForOutput(socket, hasExpectedMidiPacket, 5_000)
  }
  assert(output.includes('midi_data 144 0 64 96 256'), 'MIDI.get did not receive injected NoteOn packet')
  assert(output.includes('(144, 64, 96)'), 'MIDI packet data bytes were not normalized')

  const finalStatus = await waitForPythonActive(socket, false)
  assert(finalStatus?.active === false, 'MIDI smoke script did not exit')

  console.log('[micropython-smoke] MIDI ok')
}

async function smokeHidRaw(socket) {
  await stopPython(socket)

  const result = await runPythonText(socket, {
    name: 'hid_raw_smoke.py',
    text: [
      'import MatrixOS',
      'HID = MatrixOS.HID',
      'SYS = MatrixOS.SYS',
      'ready = False',
      'done = False',
      '',
      'def loop():',
      '    global ready, done',
      '    if not ready:',
      '        ready = True',
      '        print("hid_raw_ready")',
      '    if done:',
      '        return',
      '    data = HID.RawHID.get(0)',
      '    if data is not None:',
      '        print("hid_raw_data", data)',
      '        HID.RawHID.send(b"pong")',
      '        done = True',
      '        SYS.exit_app()',
      '',
    ].join('\n'),
  })
  assert(result.ok, 'RawHID smoke script did not start')

  const status = await waitForPythonActive(socket, true)
  assert(status?.active === true && status.mode === 'app', 'RawHID smoke script is not active')

  await waitForOutput(socket, (text) => text.includes('hid_raw_ready'))
  const sent = await rpcCall(socket, 'hid.send', { kind: 'rawhid', payload: '01 02 03 04' })
  assert(sent.ok, 'hid.send failed')

  const hasExpectedRawHidData = (text) => text.includes("hid_raw_data b'\\x01\\x02\\x03\\x04'")

  let output = await waitForOutput(socket, hasExpectedRawHidData, 3_000)
  if (!hasExpectedRawHidData(output)) {
    const retried = await rpcCall(socket, 'hid.send', { kind: 'rawhid', payload: '01 02 03 04' })
    assert(retried.ok, 'hid.send retry failed')
    output = await waitForOutput(socket, hasExpectedRawHidData, 5_000)
  }
  assert(output.includes("b'\\x01\\x02\\x03\\x04'"), 'RawHID get did not receive injected HID data')

  const finalStatus = await waitForPythonActive(socket, false)
  assert(finalStatus?.active === false, 'RawHID smoke script did not exit')

  console.log('[micropython-smoke] RawHID ok')
}

async function smokeExceptionReporting(socket) {
  await stopPython(socket)

  const result = await runPythonText(socket, {
    name: 'exception_smoke.py',
    text: [
      'print("exception_smoke_ready")',
      'def explode():',
      '    raise RuntimeError("top boom")',
      'explode()',
      '',
    ].join('\n'),
  })

  assert(result.ok, 'exception smoke script did not start')
  const finalStatus = await waitForPythonActive(socket, false, 10_000)
  assert(finalStatus?.active === false, 'exception smoke script did not exit after error')

  const output = await waitForOutput(socket, (text) => text.includes('RuntimeError: top boom'))
  assert(output.includes('Traceback (most recent call last):'), 'exception traceback header missing')
  assert(output.includes('exception_smoke.py'), 'exception traceback filename missing')
  assert(output.includes('line 3') || output.includes('line 4'), 'exception traceback line number missing')
  assert(output.includes('RuntimeError: top boom'), 'exception message missing')

  console.log('[micropython-smoke] exception reporting ok')
}

async function smokeApiIntrospection(socket) {
  await stopPython(socket)

  const name = 'api_introspection.py'
  const filePath = join(repoRoot, 'Applications', 'Python', 'examples', 'api_introspection', 'main.py')
  const text = readFileSync(filePath, 'utf8')

  const result = await runPythonText(socket, { name, text }, 20_000)
  assert(result.ok, 'api_introspection.py did not start')

  await waitForPythonActive(socket, false, 10_000)

  const output = outputText(await rpcCall(socket, 'python.getOutput', { last: 20 }))
  assert(output.includes('MatrixOS MicroPython API Introspection'), 'API introspection banner missing')
  assert(output.includes('API introspection ok'), `API introspection did not complete. Output:\n${output}`)

  console.log('[micropython-smoke] api_introspection.py ok')
}

async function smokeLifecycleStress(socket) {
  await stopPython(socket)
  await rpcCall(socket, 'input.releaseAll').catch(() => {})

  for (let i = 0; i < 6; i++) {
    const result = await runPythonText(socket, {
      name: `lifecycle_short_${i}.py`,
      text: [
        'import MatrixOS',
        `print("lifecycle_short_start", ${i})`,
        'MatrixOS.LED.clear()',
        `MatrixOS.LED.set_xy(${i % 8}, ${Math.floor(i / 8)}, 0x102030)`,
        'MatrixOS.LED.update()',
        `print("lifecycle_short_done", ${i})`,
        '',
      ].join('\n'),
    }, 20_000)
    assert(result.ok, `lifecycle short script ${i} did not start`)
    const status = await waitForPythonActive(socket, false, 10_000)
    assert(status?.active === false, `lifecycle short script ${i} did not exit`)
    const output = await waitForOutput(socket, (text) => text.includes(`lifecycle_short_done ${i}`), 3_000)
    assert(output.includes(`lifecycle_short_done ${i}`), `lifecycle short script ${i} output missing`)
    const ping = await rpcCall(socket, 'session.ping', {}, 3_000)
    assert(ping.ok, `session.ping failed after lifecycle short script ${i}`)
  }

  const invalidResult = await runPythonText(socket, {
    name: 'lifecycle_invalid_period.py',
    text: [
      'import MatrixOS',
      'try:',
      '    MatrixOS.ColorEffects.rainbow(0)',
      'except ValueError:',
      '    print("invalid_period_caught")',
      'else:',
      '    print("invalid_period_not_caught")',
      '',
    ].join('\n'),
  }, 20_000)
  assert(invalidResult.ok, 'invalid period script did not start')
  await waitForPythonActive(socket, false, 10_000)
  let output = await waitForOutput(socket, (text) => text.includes('invalid_period_caught'), 3_000)
  assert(output.includes('invalid_period_caught'), `ColorEffects period=0 did not raise ValueError. Output:\n${output}`)
  assert(!output.includes('invalid_period_not_caught'), `ColorEffects period=0 was accepted. Output:\n${output}`)

  const recoveryResult = await runPythonText(socket, {
    name: 'lifecycle_recovery.py',
    text: [
      'print("lifecycle_recovery_ok")',
      '',
    ].join('\n'),
  }, 20_000)
  assert(recoveryResult.ok, 'recovery script did not start after invalid period')
  await waitForPythonActive(socket, false, 10_000)
  output = await waitForOutput(socket, (text) => text.includes('lifecycle_recovery_ok'), 3_000)
  assert(output.includes('lifecycle_recovery_ok'), 'recovery script output missing after invalid period')

  const longResult = await runPythonText(socket, {
    name: 'lifecycle_stop.py',
    text: [
      'import MatrixOS',
      'SYS = MatrixOS.SYS',
      'print("lifecycle_stop_ready")',
      'running = True',
      'def loop():',
      '    SYS.sleep_ms(10)',
      '',
    ].join('\n'),
  }, 20_000)
  assert(longResult.ok, 'lifecycle stop script did not start')
  await waitForOutput(socket, (text) => text.includes('lifecycle_stop_ready'), 5_000)
  const activeStatus = await waitForPythonActive(socket, true, 5_000)
  assert(activeStatus?.active === true && activeStatus.mode === 'app', 'lifecycle stop script did not stay active')
  const activeDebug = await rpcCall(socket, 'python.debug', {}, 3_000)
  assert(activeDebug?.active === true, `python.debug did not report active app: ${JSON.stringify(activeDebug)}`)
  assert(activeDebug?.runtime?.initialized === true, `python.debug runtime was not initialized: ${JSON.stringify(activeDebug)}`)
  assert(activeDebug?.runtime?.heapSize > 0, `python.debug heapSize missing: ${JSON.stringify(activeDebug)}`)
  assert(activeDebug?.runtime?.heapTotal > 0, `python.debug heapTotal missing: ${JSON.stringify(activeDebug)}`)
  assert(activeDebug?.scriptPath?.endsWith('lifecycle_stop.py'), `python.debug scriptPath missing active script: ${JSON.stringify(activeDebug)}`)
  assert(activeDebug?.scripts?.entry?.endsWith('lifecycle_stop.py'), `python.debug scripts.entry missing staged script: ${JSON.stringify(activeDebug)}`)
  assert(
    activeDebug?.scripts?.files?.some((file) => file.path?.endsWith('lifecycle_stop.py') && file.entry === true && file.size > 0),
    `python.debug scripts.files missing staged entry metadata: ${JSON.stringify(activeDebug)}`,
  )
  await stopPython(socket)
  const stoppedStatus = await rpcCall(socket, 'python.status', {}, 3_000)
  assert(stoppedStatus.active === false, 'python.status did not report inactive after lifecycle stop')
  const stoppedDebug = await rpcCall(socket, 'python.debug', {}, 3_000)
  assert(stoppedDebug?.active === false, `python.debug did not report inactive after lifecycle stop: ${JSON.stringify(stoppedDebug)}`)
  assert(stoppedDebug?.runtime?.initialized === false, `python.debug runtime stayed initialized after lifecycle stop: ${JSON.stringify(stoppedDebug)}`)
  const finalPing = await rpcCall(socket, 'session.ping', {}, 3_000)
  assert(finalPing.ok, 'session.ping failed after lifecycle stop')

  console.log('[micropython-smoke] lifecycle stress ok')
}

async function smokeUiInteraction(socket) {
  await stopPython(socket)

  const result = await runPythonText(socket, {
    name: 'ui_interaction_smoke.py',
    text: [
      'import MatrixOS',
      'UI = MatrixOS.UI',
      'Input = MatrixOS.Input',
      'SYS = MatrixOS.SYS',
      'FUNCTION_KEY = Input.function_key()',
      'Input.clear()',
      'opened = False',
      'ui_done = False',
      'parent_leak = False',
      'reported_done = False',
      '',
      'def loop():',
      '    global opened, ui_done, parent_leak, reported_done',
      '    if not opened:',
      '        opened = True',
      '        ui = UI.UI("Interaction", 0x00FFFF, True)',
      '        button = UI.Button("Button", 0xFF0000)',
      '        button.on_press(lambda: print("button_press"))',
      '        button.on_hold(lambda: print("button_hold"))',
      '        ui.add(button, (0, 0))',
      '        selector = UI.Selector((2, 1), 2)',
      '        selector.set_value(0)',
      '        selector.on_change(lambda value: print("selector_change", value))',
      '        ui.add(selector, (1, 0))',
      '        def input_handler(event):',
      '            if event.get("id") == FUNCTION_KEY:',
      '                keypad = event.get("keypad")',
      '                if keypad and keypad.get("released"):',
      '                    print("ui_fn_release")',
      '                    ui.exit()',
      '                return True',
      '            return False',
      '        ui.set_input_handler(input_handler)',
      '        print("ui_ready")',
      '        ui.start()',
      '        ui_done = True',
      '        print("ui_closed")',
      '        Input.clear()',
      '        return',
      '    event = Input.get_event()',
      '    while event is not None:',
      '        if ui_done and event.get("id") == FUNCTION_KEY and event.get("keypad", {}).get("released"):',
      '            parent_leak = True',
      '            print("parent_fn_release_leak")',
      '        event = Input.get_event()',
      '    if ui_done and not reported_done:',
      '        reported_done = True',
      '        print("ui_interaction_done")',
      '        SYS.exit_app()',
      '',
    ].join('\n'),
  })
  assert(result.ok, 'UI interaction smoke script did not start')

  const status = await waitForPythonActive(socket, true)
  assert(status?.active === true && status.mode === 'app', 'UI interaction smoke script is not active')

  await waitForOutput(socket, (text) => text.includes('ui_ready'))

  await rpcCall(socket, 'input.execute', {
    events: [
      { input: 'grid:0,0', action: 'Press' },
      { input: 'grid:0,0', action: 'Release', atMs: 50 },
    ],
  })
  await waitForOutput(socket, (text) => text.includes('button_press'))

  await rpcCall(socket, 'input.execute', {
    events: [
      { input: 'grid:0,0', action: 'Press' },
      { input: 'grid:0,0', action: 'Release', atMs: 1000 },
    ],
  })
  await waitForOutput(socket, (text) => text.includes('button_hold'))

  await rpcCall(socket, 'input.execute', {
    events: [
      { input: 'grid:2,0', action: 'Press' },
      { input: 'grid:2,0', action: 'Release' },
    ],
  })
  await waitForOutput(socket, (text) => text.includes('selector_change 1'))

  await rpcCall(socket, 'input.execute', {
    events: [
      { input: 'function', action: 'Press' },
      { input: 'function', action: 'Release', atMs: 160 },
    ],
  })
  const output = await waitForOutput(socket, (text) => text.includes('ui_interaction_done'))
  assert(output.includes('ui_fn_release'), 'UI did not consume function-key release')
  assert(output.includes('ui_closed'), 'UI did not close after function-key release')
  assert(!output.includes('parent_fn_release_leak'), 'function-key release leaked to parent app loop')

  const finalStatus = await waitForPythonActive(socket, false)
  assert(finalStatus?.active === false, 'UI interaction smoke script did not exit')

  console.log('[micropython-smoke] UI interaction ok')
}

async function smokeUiCallbackException(socket) {
  await stopPython(socket)

  const script = [
    'import MatrixOS',
    'UI = MatrixOS.UI',
    'Input = MatrixOS.Input',
    'SYS = MatrixOS.SYS',
    'Input.clear()',
    '',
    'def bad_press():',
    '    print("callback_before_raise")',
    '    raise RuntimeError("callback boom")',
    '',
    'opened = False',
    '',
    'def loop():',
    '    global opened',
    '    if opened:',
    '        return',
    '    opened = True',
    '    main_ui = UI.UI("Callback Error Smoke", MatrixOS.Color(16, 16, 16), True)',
    '    bad_button = UI.Button("Bad", MatrixOS.Color(48, 0, 0))',
    '    bad_button.on_press(bad_press)',
    '    main_ui.add(bad_button, (0, 0))',
    '    print("callback_ui_ready")',
    '    main_ui.start()',
    '    print("callback_ui_closed")',
    '    SYS.exit_app()',
    '',
  ].join('\n')

  const result = await runPythonText(socket, {
    name: 'ui_callback_exception_smoke.py',
    text: script,
  })
  assert(result.ok, 'UI callback exception smoke script did not start')

  const status = await waitForPythonActive(socket, true)
  assert(status?.active === true && status.mode === 'app', 'UI callback exception smoke script is not active')

  await waitForOutput(socket, (text) => text.includes('callback_ui_ready'))
  await rpcCall(socket, 'input.execute', {
    events: [
      { input: 'grid:0,0', action: 'Press' },
      { input: 'grid:0,0', action: 'Release' },
    ],
  })

  let output = await waitForOutput(socket, (text) => text.includes('RuntimeError: callback boom'), 2_000)
  if (!output.includes('RuntimeError: callback boom')) {
    await rpcCall(socket, 'input.execute', {
      events: [
        { input: 'grid:0,0', action: 'Press' },
        { input: 'grid:0,0', action: 'Release' },
      ],
    })
    output = await waitForOutput(socket, (text) => text.includes('RuntimeError: callback boom'))
  }
  assert(output.includes('callback_before_raise'), 'callback did not run before raising')
  assert(output.includes('Traceback (most recent call last):'), 'callback exception traceback header missing')
  assert(output.includes('ui_callback_exception_smoke.py'), 'callback exception traceback filename missing')

  await new Promise((resolve) => setTimeout(resolve, 150))
  await rpcCall(socket, 'input.execute', {
    events: [
      { input: 'function', action: 'Press' },
      { input: 'function', action: 'Release', atMs: 120 },
    ],
  })

  output = await waitForOutput(socket, (text) => text.includes('callback_ui_closed'))
  const debugStatus = await rpcCall(socket, 'python.status').catch((error) => ({ error: String(error?.message || error) }))
  assert(
    output.includes('callback_ui_closed'),
    `UI did not stay responsive after callback exception. Status: ${JSON.stringify(debugStatus)} Output:\n${output}`,
  )

  const finalStatus = await waitForPythonActive(socket, false)
  assert(finalStatus?.active === false, 'UI callback exception smoke script did not exit')

  console.log('[micropython-smoke] UI callback exception ok')
}

async function smokeUiTextScrollHold(socket) {
  await stopPython(socket)

  const result = await runPythonText(socket, {
    name: 'ui_text_scroll_hold_smoke.py',
    text: [
      'import MatrixOS',
      'UI = MatrixOS.UI',
      'Input = MatrixOS.Input',
      'SYS = MatrixOS.SYS',
      'FUNCTION_KEY = Input.function_key()',
      'Input.clear()',
      'opened = False',
      '',
      'def loop():',
      '    global opened',
      '    if opened:',
      '        return',
      '    opened = True',
      '    ui = UI.UI("TextScroll Hold", 0x101010, True)',
      '    scroll_button = UI.Button("I", 0x00FF00)',
      '    scroll_button.on_press(lambda: print("scroll_button_press"))',
      '    ui.add(scroll_button, (0, 0))',
      '    close_button = UI.Button("Close", 0x0000FF)',
      '    close_button.on_press(lambda: ui.exit())',
      '    ui.add(close_button, (1, 0))',
      '    def input_handler(event):',
      '        if event.get("id") == FUNCTION_KEY:',
      '            keypad = event.get("keypad")',
      '            if keypad and keypad.get("released"):',
      '                ui.exit()',
      '            return True',
      '        return False',
      '    ui.set_input_handler(input_handler)',
      '    print("text_scroll_hold_ready")',
      '    ui.start()',
      '    print("text_scroll_hold_closed")',
      '    SYS.exit_app()',
      '',
    ].join('\n'),
  })
  assert(result.ok, 'UI text-scroll hold smoke script did not start')

  const status = await waitForPythonActive(socket, true)
  assert(status?.active === true && status.mode === 'app', 'UI text-scroll hold smoke script is not active')

  await waitForOutput(socket, (text) => text.includes('text_scroll_hold_ready'))
  await rpcCall(socket, 'input.execute', {
    events: [
      { input: 'grid:0,0', action: 'Press' },
      { input: 'grid:0,0', action: 'Release', atMs: 700 },
    ],
  })

  const textFrame = await waitForLedFrame(socket, (frame) => (
    Array.isArray(frame.grid) &&
    frame.grid.filter((color) => color !== '00000000').length > 2
  ), 3_000)
  assert(
    textFrame?.grid?.filter((color) => color !== '00000000').length > 2,
    'button hold did not render TextScroll frame',
  )

  const activeAfterHold = await waitForPythonActive(socket, true, 1_000)
  assert(activeAfterHold?.active === true, 'TextScroll hold unexpectedly exited the app')

  const restoredFrame = await waitForLedFrame(socket, (frame) => (
    Array.isArray(frame.grid) &&
    frame.grid.filter((color) => color !== '00000000').length <= 2
  ), 8_000)
  assert(
    restoredFrame?.grid?.filter((color) => color !== '00000000').length <= 2,
    'TextScroll hold did not return to the parent UI frame',
  )
  await new Promise((resolve) => setTimeout(resolve, 2_000))

  let closedOutput = ''
  for (let attempt = 0; attempt < 3; attempt++) {
    await rpcCall(socket, 'input.execute', {
      events: [
        { input: 'grid:1,0', action: 'Press' },
        { input: 'grid:1,0', action: 'Release', atMs: 160 },
      ],
    })
    closedOutput = await waitForOutput(socket, (text) => text.includes('text_scroll_hold_closed'), 2_000)
    if (closedOutput.includes('text_scroll_hold_closed')) break
    await new Promise((resolve) => setTimeout(resolve, 500))
  }
  assert(closedOutput.includes('text_scroll_hold_closed'), 'UI did not stay responsive after TextScroll hold')

  const finalStatus = await waitForPythonActive(socket, false)
  assert(finalStatus?.active === false, 'UI text-scroll hold smoke script did not exit')

  console.log('[micropython-smoke] UI text-scroll hold ok')
}

async function smokeUiSelectorTextScrollHold(socket) {
  await stopPython(socket)

  const result = await runPythonText(socket, {
    name: 'ui_selector_text_scroll_hold_smoke.py',
    text: [
      'import MatrixOS',
      'UI = MatrixOS.UI',
      'Input = MatrixOS.Input',
      'SYS = MatrixOS.SYS',
      'FUNCTION_KEY = Input.function_key()',
      'Input.clear()',
      'opened = False',
      '',
      'def loop():',
      '    global opened',
      '    if opened:',
      '        return',
      '    opened = True',
      '    ui = UI.UI("Selector Hold", 0x101010, True)',
      '    selector = UI.Selector((1, 1), 1)',
      '    selector.set_color(0x00FF00)',
      '    selector.set_name_func(lambda index: "Selector Long Name")',
      '    ui.add(selector, (0, 0))',
      '    close_button = UI.Button("Close", 0x0000FF)',
      '    close_button.on_press(lambda: ui.exit())',
      '    ui.add(close_button, (1, 0))',
      '    print("selector_text_scroll_hold_ready")',
      '    ui.start()',
      '    print("selector_text_scroll_hold_closed")',
      '    SYS.exit_app()',
      '',
    ].join('\n'),
  })
  assert(result.ok, 'UI selector text-scroll hold smoke script did not start')

  const status = await waitForPythonActive(socket, true)
  assert(status?.active === true && status.mode === 'app', 'UI selector text-scroll hold smoke script is not active')

  await waitForOutput(socket, (text) => text.includes('selector_text_scroll_hold_ready'))
  await rpcCall(socket, 'input.execute', {
    events: [
      { input: 'grid:0,0', action: 'Press' },
      { input: 'grid:0,0', action: 'Release', atMs: 700 },
    ],
  })

  const textFrame = await waitForLedFrame(socket, (frame) => (
    Array.isArray(frame.grid) &&
    frame.grid.filter((color) => color !== '00000000').length > 2
  ), 3_000)
  assert(
    textFrame?.grid?.filter((color) => color !== '00000000').length > 2,
    'selector hold did not render TextScroll frame',
  )

  const activeAfterHold = await waitForPythonActive(socket, true, 1_000)
  assert(activeAfterHold?.active === true, 'Selector TextScroll hold unexpectedly exited the app')

  const restoredFrame = await waitForLedFrame(socket, (frame) => (
    Array.isArray(frame.grid) &&
    frame.grid.filter((color) => color !== '00000000').length <= 2
  ), 8_000)
  assert(
    restoredFrame?.grid?.filter((color) => color !== '00000000').length <= 2,
    'Selector TextScroll hold did not return to the parent UI frame',
  )

  await new Promise((resolve) => setTimeout(resolve, 2_000))
  let output = ''
  for (let attempt = 0; attempt < 3; attempt++) {
    await rpcCall(socket, 'input.execute', {
      events: [
        { input: 'grid:1,0', action: 'Press' },
        { input: 'grid:1,0', action: 'Release', atMs: 160 },
      ],
    })
    output = await waitForOutput(socket, (text) => text.includes('selector_text_scroll_hold_closed'), 2_000)
    if (output.includes('selector_text_scroll_hold_closed')) break
    await new Promise((resolve) => setTimeout(resolve, 500))
  }
  assert(output.includes('selector_text_scroll_hold_closed'), 'UI selector TextScroll smoke did not close')

  const finalStatus = await waitForPythonActive(socket, false)
  assert(finalStatus?.active === false, 'UI selector text-scroll hold smoke script did not exit')

  console.log('[micropython-smoke] UI selector text-scroll hold ok')
}

async function smokeUiNumberTextScrollHold(socket) {
  await stopPython(socket)

  const result = await runPythonText(socket, {
    name: 'ui_number_text_scroll_hold_smoke.py',
    text: [
      'import MatrixOS',
      'UI = MatrixOS.UI',
      'Input = MatrixOS.Input',
      'SYS = MatrixOS.SYS',
      'FUNCTION_KEY = Input.function_key()',
      'Input.clear()',
      'opened = False',
      '',
      'def loop():',
      '    global opened',
      '    if opened:',
      '        return',
      '    opened = True',
      '    ui = UI.UI("Number Hold", 0x101010, True)',
      '    number = UI.Number(1)',
      '    number.set_name("Number Long Name")',
      '    number.set_value(7)',
      '    number.set_color(0x00FF00)',
      '    ui.add(number, (0, 0))',
      '    start_ms = SYS.millis()',
      '    def auto_close():',
      '        if SYS.millis() - start_ms > 5000:',
      '            ui.exit()',
      '    ui.set_global_loop_func(auto_close)',
      '    print("number_text_scroll_hold_ready")',
      '    ui.start()',
      '    print("number_text_scroll_hold_closed")',
      '    SYS.exit_app()',
      '',
    ].join('\n'),
  })
  assert(result.ok, 'UI number text-scroll hold smoke script did not start')

  const status = await waitForPythonActive(socket, true)
  assert(status?.active === true && status.mode === 'app', 'UI number text-scroll hold smoke script is not active')

  await waitForOutput(socket, (text) => text.includes('number_text_scroll_hold_ready'))
  await rpcCall(socket, 'input.execute', {
    events: [
      { input: 'grid:0,0', action: 'Press' },
      { input: 'grid:0,0', action: 'Release', atMs: 700 },
    ],
  })

  const textFrame = await waitForLedFrame(socket, (frame) => (
    Array.isArray(frame.grid) &&
    frame.grid.filter((color) => color !== '00000000').length > 8
  ), 3_000)
  assert(
    textFrame?.grid?.filter((color) => color !== '00000000').length > 8,
    'number hold did not render TextScroll frame',
  )

  const activeAfterHold = await waitForPythonActive(socket, true, 1_000)
  assert(activeAfterHold?.active === true, 'Number TextScroll hold unexpectedly exited the app')

  const restoredFrame = await waitForLedFrame(socket, (frame) => (
    Array.isArray(frame.grid) &&
    frame.grid.filter((color) => color !== '00000000').length <= 8
  ), 8_000)
  assert(
    restoredFrame?.grid?.filter((color) => color !== '00000000').length <= 8,
    'Number TextScroll hold did not return to the parent UI frame',
  )

  const output = await waitForOutput(socket, (text) => text.includes('number_text_scroll_hold_closed'), 8_000)
  assert(output.includes('number_text_scroll_hold_closed'), 'UI number TextScroll smoke did not close')

  await rpcCall(socket, 'input.releaseAll').catch(() => {})
  await new Promise((resolve) => setTimeout(resolve, 100))
  const finalStatus = await waitForPythonActive(socket, false)
  assert(finalStatus?.active === false, 'UI number text-scroll hold smoke script did not exit')

  console.log('[micropython-smoke] UI number text-scroll hold ok')
}

async function smokeUiFunctionHoldExit(socket) {
  await stopPython(socket)

  const result = await runPythonText(socket, {
    name: 'ui_fn_hold_smoke.py',
    text: [
      'import MatrixOS',
      'UI = MatrixOS.UI',
      'Input = MatrixOS.Input',
      'SYS = MatrixOS.SYS',
      'FUNCTION_KEY = Input.function_key()',
      'Input.clear()',
      'opened = False',
      '',
      'def loop():',
      '    global opened',
      '    if opened:',
      '        return',
      '    opened = True',
      '    ui = UI.UI("Hold Exit", 0x00FFFF, True)',
      '    def input_handler(event):',
      '        if event.get("id") != FUNCTION_KEY:',
      '            return False',
      '        keypad = event.get("keypad")',
      '        if keypad and keypad.get("hold"):',
      '            print("ui_fn_hold_exit")',
      '            SYS.exit_app()',
      '        return True',
      '    ui.set_input_handler(input_handler)',
      '    print("ui_hold_ready")',
      '    ui.start()',
      '',
    ].join('\n'),
  })
  assert(result.ok, 'UI function-hold smoke script did not start')

  const status = await waitForPythonActive(socket, true)
  assert(status?.active === true && status.mode === 'app', 'UI function-hold smoke script is not active')

  await waitForOutput(socket, (text) => text.includes('ui_hold_ready'))
  await rpcCall(socket, 'input.execute', {
    events: [
      { input: 'function', action: 'Press' },
      { input: 'function', action: 'Release', atMs: 700 },
    ],
  })

  const output = await waitForOutput(socket, (text) => text.includes('ui_fn_hold_exit'))
  assert(output.includes('ui_fn_hold_exit'), 'function-key hold did not reach app policy handler')

  const finalStatus = await waitForPythonActive(socket, false)
  assert(finalStatus?.active === false, 'UI function-hold smoke script did not exit')

  console.log('[micropython-smoke] UI function hold exit ok')
}

async function smokeUiColorPicker(socket) {
  await stopPython(socket)

  const result = await runPythonText(socket, {
    name: 'ui_color_picker_smoke.py',
    text: [
      'import MatrixOS',
      'UI = MatrixOS.UI',
      'SYS = MatrixOS.SYS',
      '',
      'started = False',
      '',
      'def loop():',
      '    global started',
      '    if started:',
      '        return',
      '    started = True',
      '    print("color_picker_ready")',
      '    picked = UI.color_picker(0xFF0000, False)',
      '    print("color_picker_result", picked is not None, isinstance(picked, int))',
      '    SYS.exit_app()',
      '',
    ].join('\n'),
  })
  assert(result.ok, 'UI color picker smoke script did not start')

  const status = await waitForPythonActive(socket, true)
  assert(status?.active === true && status.mode === 'app', 'UI color picker smoke script is not active')

  await waitForOutput(socket, (text) => text.includes('color_picker_ready'))
  await rpcCall(socket, 'input.execute', {
    events: [
      { input: 'grid:3,3', action: 'Press' },
      { input: 'grid:3,3', action: 'Release', atMs: 120 },
    ],
  })

  const output = await waitForOutput(socket, (text) => text.includes('color_picker_result True True'))
  assert(output.includes('color_picker_result True True'), `UI color picker did not return a color. Output:\n${output}`)

  const finalStatus = await waitForPythonActive(socket, false)
  assert(finalStatus?.active === false, 'UI color picker smoke script did not exit')

  console.log('[micropython-smoke] UI color picker ok')
}

async function smokeUiNumberSelector(socket) {
  await stopPython(socket)

  const result = await runPythonText(socket, {
    name: 'ui_number_selector_smoke.py',
    text: [
      'import MatrixOS',
      'UI = MatrixOS.UI',
      'SYS = MatrixOS.SYS',
      '',
      'started = False',
      '',
      'def loop():',
      '    global started',
      '    if started:',
      '        return',
      '    started = True',
      '    print("number_selector_ready")',
      '    picked = UI.number_selector(10, 0x00FFFF, "Number Selector", 0, 99)',
      '    print("number_selector_result", picked)',
      '    SYS.exit_app()',
      '',
    ].join('\n'),
  })
  assert(result.ok, 'UI number selector smoke script did not start')

  const status = await waitForPythonActive(socket, true)
  assert(status?.active === true && status.mode === 'app', 'UI number selector smoke script is not active')

  await waitForOutput(socket, (text) => text.includes('number_selector_ready'))
  await rpcCall(socket, 'input.execute', {
    events: [
      { input: 'grid:5,7', action: 'Press' },
      { input: 'grid:5,7', action: 'Release', atMs: 120 },
      { input: 'function', action: 'Press', atMs: 260 },
      { input: 'function', action: 'Release', atMs: 420 },
    ],
  })

  const output = await waitForOutput(socket, (text) => text.includes('number_selector_result 15'))
  assert(output.includes('number_selector_result 15'), `UI number selector did not return selected value. Output:\n${output}`)

  const finalStatus = await waitForPythonActive(socket, false)
  assert(finalStatus?.active === false, 'UI number selector smoke script did not exit')

  console.log('[micropython-smoke] UI number selector ok')
}

async function smokeExample(socket, name) {
  await stopPython(socket)
  await rpcCall(socket, 'input.releaseAll').catch(() => {})

  const filePath = join(repoRoot, 'Applications', 'Python', 'examples', name)
  const text = readFileSync(filePath, 'utf8')

  async function stageAndRun() {
    const staged = await rpcCall(socket, 'python.stage', { name, text })
    assert(staged.ok && staged.size === text.length, `${name} stage failed`)
    return runPythonStaged(socket)
  }

  let run
  try {
    run = await stageAndRun()
  } catch (error) {
    await stopPython(socket)
    run = await stageAndRun()
  }
  assert(run.ok, `${name} runStaged failed`)

  const status = await waitForPythonActive(socket, true)
  assert(status?.active === true && status.mode === 'app', `${name} did not become active app`)

  await rpcCall(socket, 'input.releaseAll').catch(() => {})
  await stopPython(socket)
  console.log(`[micropython-smoke] ${name} ok`)
}

async function runExample(socket, name) {
  await stopPython(socket)
  await rpcCall(socket, 'input.releaseAll').catch(() => {})

  const scriptName = `${name}.py`
  const filePath = join(repoRoot, 'Applications', 'Python', 'examples', name, 'main.py')
  const text = readFileSync(filePath, 'utf8')
  const staged = await rpcCall(socket, 'python.stage', { name: scriptName, text })
  assert(staged.ok && staged.size === text.length, `${name} stage failed`)

  const run = await runPythonStaged(socket)
  assert(run.ok, `${name} runStaged failed`)

  const status = await waitForPythonActive(socket, true)
  assert(status?.active === true && status.mode === 'app', `${name} did not become active app`)
}

async function waitForLed(socket, id, expectedColor, timeoutMs = 3_000) {
  const deadline = Date.now() + timeoutMs
  let led

  while (Date.now() < deadline) {
    led = await rpcCall(socket, 'led.get', { id })
    if (led.color === expectedColor) return led
    await new Promise((resolve) => setTimeout(resolve, 100))
  }

  return led
}

async function waitForLedNot(socket, id, excludedColor, timeoutMs = 3_000) {
  const deadline = Date.now() + timeoutMs
  let led

  while (Date.now() < deadline) {
    led = await rpcCall(socket, 'led.get', { id })
    if (led.color !== excludedColor) return led
    await new Promise((resolve) => setTimeout(resolve, 100))
  }

  return led
}

async function waitForLedFrame(socket, predicate, timeoutMs = 5_000) {
  const deadline = Date.now() + timeoutMs
  let frame

  while (Date.now() < deadline) {
    frame = await rpcCall(socket, 'led.getFrame')
    if (predicate(frame)) return frame
    await new Promise((resolve) => setTimeout(resolve, 100))
  }

  return frame
}

async function waitForNvsEntry(socket, hash, predicate, timeoutMs = 3_000) {
  const deadline = Date.now() + timeoutMs
  let entry = null

  while (Date.now() < deadline) {
    try {
      entry = await rpcCall(socket, 'storage.nvs.get', { hash })
      if (predicate(entry)) return entry
    } catch {}
    await new Promise((resolve) => setTimeout(resolve, 100))
  }

  return entry
}

async function waitForNvsRaw(socket, hash, expectedRawBytes, timeoutMs = 3_000) {
  return waitForNvsEntry(socket, hash, (entry) => entry.rawBytes === expectedRawBytes, timeoutMs)
}

async function setNvsRaw(socket, keyText, rawBytes) {
  const key = await rpcCall(socket, 'storage.nvs.computeHash', { text: keyText })
  const result = await rpcCall(socket, 'storage.nvs.set', { hash: key.hash, value: rawBytes, encoding: 'hex' })
  assert(result.ok, `failed to seed NVS ${keyText}`)
  return key.hash
}

function pixelArtRaw(pixels = []) {
  const width = 10
  const bytes = new Array(width * 8 * 3).fill(0)
  for (const pixel of pixels) {
    const index = (pixel.y * width + pixel.x + 1) * 3
    const color = Number.parseInt(pixel.color, 16)
    bytes[index] = (color >> 16) & 0xFF
    bytes[index + 1] = (color >> 8) & 0xFF
    bytes[index + 2] = color & 0xFF
  }
  return bytes.map((byte) => byte.toString(16).padStart(2, '0').toUpperCase()).join(' ')
}

function findAdjacentSameColorCell(grid) {
  for (let y = 0; y < 8; y++) {
    for (let x = 0; x < 8; x++) {
      const color = grid[y * 8 + x]
      if (!color || color === '00000000') continue
      if (x < 7 && grid[y * 8 + x + 1] === color) return { x, y }
      if (y < 7 && grid[(y + 1) * 8 + x] === color) return { x, y }
    }
  }
  return null
}

function sameGrid(a, b) {
  return Array.isArray(a) && Array.isArray(b) && a.length === b.length && a.every((color, index) => color === b[index])
}

async function waitForStableSameGameBoard(socket, options = {}) {
  const requireFull = options.requireFull !== false
  const requireAdjacent = options.requireAdjacent !== false
  let previousGrid = null
  const deadline = Date.now() + 7_000

  while (Date.now() < deadline) {
    const frame = await rpcCall(socket, 'led.getFrame')
    const grid = frame.grid
    const hasGrid = Array.isArray(grid) && grid.length >= 64
    const fullEnough = !requireFull || (hasGrid && grid.every((color) => color !== '00000000'))
    const adjacentEnough = !requireAdjacent || (hasGrid && findAdjacentSameColorCell(grid) !== null)
    if (hasGrid && fullEnough && adjacentEnough && sameGrid(grid, previousGrid)) return frame
    previousGrid = Array.isArray(grid) ? [...grid] : null
    await new Promise((resolve) => setTimeout(resolve, 250))
  }

  return rpcCall(socket, 'led.getFrame')
}

async function smokePixelArtInteraction(socket) {
  const artHash = await setNvsRaw(socket, 'Python Pixel Art grid', pixelArtRaw())
  await runExample(socket, 'pixel_art')

  await waitForLed(socket, 'grid:0,0', 'FF000000')
  let led = await waitForLed(socket, 'grid:1,0', 'FF800000')
  assert(led.color === 'FF800000', 'pixel_art.py did not render picker color row')

  await rpcCall(socket, 'input.execute', {
    events: [
      { input: 'grid:1,0', action: 'Press' },
      { input: 'grid:1,0', action: 'Release', atMs: 120 },
    ],
  })

  await rpcCall(socket, 'input.execute', {
    events: [
      { input: 'function', action: 'Press' },
      { input: 'function', action: 'Release', atMs: 160 },
    ],
  })
  led = await waitForLed(socket, 'grid:1,0', '00000000')
  assert(led.color === '00000000', 'pixel_art.py function-key release did not hide picker')

  await rpcCall(socket, 'input.execute', {
    events: [
      { input: 'grid:2,2', action: 'Press' },
      { input: 'grid:2,2', action: 'Release', atMs: 120 },
    ],
  })
  led = await waitForLed(socket, 'grid:2,2', 'FF800000')
  assert(led.color === 'FF800000', 'pixel_art.py did not paint selected color to grid')
  await rpcCall(socket, 'input.execute', {
    events: [
      { input: 'touchbar:left:2', action: 'Press' },
      { input: 'touchbar:left:2', action: 'Release', atMs: 120 },
    ],
  })
  led = await waitForLed(socket, 'xy:-1,2', 'FF800000')
  assert(led.color === 'FF800000', 'pixel_art.py did not paint selected color to left touchbar')

  await rpcCall(socket, 'input.execute', {
    events: [
      { input: 'touchbar:right:3', action: 'Press' },
      { input: 'touchbar:right:3', action: 'Release', atMs: 120 },
    ],
  })
  led = await waitForLed(socket, 'xy:8,3', 'FF800000')
  assert(led.color === 'FF800000', 'pixel_art.py did not paint selected color to right touchbar')

  const expectedArt = pixelArtRaw([
    { x: 2, y: 2, color: 'FF8000' },
    { x: -1, y: 2, color: 'FF8000' },
    { x: 8, y: 3, color: 'FF8000' },
  ])
  const expectedArtPrefix = expectedArt.split(' ').slice(0, 64).join(' ')
  const storedArt = await waitForNvsEntry(socket, artHash, (entry) => (
    entry.size === 10 * 8 * 3 && entry.rawBytes === expectedArtPrefix
  ))
  assert(
    storedArt?.size === 10 * 8 * 3 && storedArt.rawBytes === expectedArtPrefix,
    `pixel_art.py did not persist painted grid to NVS: size=${storedArt?.size ?? 'missing'} raw=${storedArt?.rawBytes ?? 'missing'}`,
  )

  await stopPython(socket)
  const clearResult = await runPythonText(socket, {
    name: 'clear_led.py',
    text: [
      'import MatrixOS',
      'MatrixOS.LED.clear()',
      'MatrixOS.LED.update()',
      '',
    ].join('\n'),
  })
  assert(clearResult.ok, 'clear_led.py did not run')
  await waitForPythonActive(socket, false)
  led = await waitForLed(socket, 'grid:2,2', '00000000')
  assert(led.color === '00000000', 'clear_led.py did not clear painted pixel before restore check')

  await runExample(socket, 'pixel_art')
  led = await waitForLed(socket, 'grid:2,2', 'FF800000')
  assert(led.color === 'FF800000', 'pixel_art.py did not restore painted grid from NVS')
  led = await waitForLed(socket, 'xy:-1,2', 'FF800000')
  assert(led.color === 'FF800000', 'pixel_art.py did not restore left touchbar art from NVS')
  led = await waitForLed(socket, 'xy:8,3', 'FF800000')
  assert(led.color === 'FF800000', 'pixel_art.py did not restore right touchbar art from NVS')

  await holdFunctionKeyToExitExample(socket, 'pixel_art.py')
  console.log('[micropython-smoke] pixel_art.py interaction ok')
}

async function smokeSameGameInteraction(socket) {
  await runExample(socket, 'same_game')

  let frame = await waitForStableSameGameBoard(socket)

  const target = findAdjacentSameColorCell(frame.grid)
  assert(target !== null, 'same_game.py did not generate a removable adjacent group')

  await rpcCall(socket, 'input.execute', {
    events: [
      { input: `grid:${target.x},${target.y}`, action: 'Press' },
      { input: `grid:${target.x},${target.y}`, action: 'Release' },
    ],
  })

  frame = await waitForLedFrame(socket, (candidate) => (
    Array.isArray(candidate.grid) &&
    candidate.grid.some((color) => color === '00000000')
  ), 3_000)
  assert(frame.grid.some((color) => color === '00000000'), 'same_game.py did not remove a matched group')

  frame = await waitForStableSameGameBoard(socket, { requireFull: false })
  const filledAfterMove = frame.grid.filter((color) => color !== '00000000').length
  assert(filledAfterMove > 0, 'same_game.py did not render board after gravity/compact')
  assert(findAdjacentSameColorCell(frame.grid) !== null, 'same_game.py did not return to a stable playable board')

  await rpcCall(socket, 'input.execute', {
    events: [
      { input: 'function', action: 'Press' },
      { input: 'function', action: 'Release', atMs: 160 },
    ],
  })
  let led = await waitForLed(socket, 'grid:7,3', 'FF000000')
  assert(led.color === 'FF000000', 'same_game.py did not enter settings on function-key short press')

  await rpcCall(socket, 'input.execute', {
    events: [
      { input: 'grid:3,0', action: 'Press' },
      { input: 'grid:3,0', action: 'Release' },
    ],
  })
  led = await waitForLed(socket, 'grid:3,0', 'FFFFFF00')
  assert(led.color === 'FFFFFF00', 'same_game.py did not update selected color count in settings')

  const key = await rpcCall(socket, 'storage.nvs.computeHash', { text: 'Python SameGame num_colors' })
  const nvsEntry = await waitForNvsRaw(socket, key.hash, '05')
  assert(nvsEntry?.rawBytes === '05', 'same_game.py did not persist num_colors to NVS')

  await rpcCall(socket, 'input.execute', {
    events: [
      { input: 'function', action: 'Release' },
      { input: 'function', action: 'Press', atMs: 180 },
      { input: 'function', action: 'Release', atMs: 340 },
    ],
  })
  led = await waitForLedNot(socket, 'grid:3,0', 'FFFFFF00')
  assert(led.color !== 'FFFFFF00', 'same_game.py did not return from settings after function-key release')

  await rpcCall(socket, 'input.execute', {
    events: [
      { input: 'function', action: 'Press' },
      { input: 'function', action: 'Release', atMs: 160 },
    ],
  })
  led = await waitForLed(socket, 'grid:7,3', 'FF000000')
  assert(led.color === 'FF000000', 'same_game.py did not re-enter settings before reset confirm')

  await rpcCall(socket, 'input.execute', {
    events: [
      { input: 'grid:7,3', action: 'Press' },
      { input: 'grid:7,3', action: 'Release', atMs: 120 },
    ],
  })
  await new Promise((resolve) => setTimeout(resolve, 200))

  await rpcCall(socket, 'input.execute', {
    events: [
      { input: 'grid:7,3', action: 'Press' },
      { input: 'grid:7,3', action: 'Release', atMs: 120 },
    ],
  })

  frame = await waitForStableSameGameBoard(socket)
  assert(frame.grid.every((color) => color !== '00000000'), 'same_game.py reset confirm did not rebuild a full board')

  await holdFunctionKeyToExitExample(socket, 'same_game.py')
  console.log('[micropython-smoke] same_game.py interaction ok')
}

async function smokeGomokuInteraction(socket) {
  const player1ColorKey = await setNvsRaw(socket, 'Python Gomoku player1_color', 'AE FF 00 00')
  const player2ColorKey = await setNvsRaw(socket, 'Python Gomoku player2_color', 'FF 00 66 00')
  await setNvsRaw(socket, 'Python Gomoku first_player', '01')
  await setNvsRaw(socket, 'Python Gomoku winning_length', '05')

  await runExample(socket, 'gomoku')
  await new Promise((resolve) => setTimeout(resolve, 300))

  await rpcCall(socket, 'input.execute', {
    events: [
      { input: 'grid:3,3', action: 'Press' },
      { input: 'grid:3,3', action: 'Release' },
    ],
  })
  let led = await waitForLedNot(socket, 'grid:3,3', '00000000')
  assert(led.color !== '00000000', 'gomoku.py did not place a stone on key release')

  await rpcCall(socket, 'input.execute', {
    events: [
      { input: 'function', action: 'Press' },
      { input: 'function', action: 'Release', atMs: 160 },
    ],
  })
  led = await waitForLed(socket, 'grid:7,3', 'FF000000')
  assert(led.color === 'FF000000', 'gomoku.py did not enter settings before reset confirm')

  await clickInput(socket, 'grid:7,3')
  await new Promise((resolve) => setTimeout(resolve, 200))
  await clickInput(socket, 'grid:7,3')

  led = await waitForLed(socket, 'grid:3,3', '00000000')
  assert(led.color === '00000000', 'gomoku.py reset confirm did not clear placed stone')

  await rpcCall(socket, 'input.execute', {
    events: [
      { input: 'function', action: 'Press' },
      { input: 'grid:0,7', action: 'Press', atMs: 40 },
      { input: 'grid:0,7', action: 'Release', atMs: 70 },
      { input: 'grid:0,0', action: 'Press', atMs: 100 },
      { input: 'grid:0,0', action: 'Release', atMs: 130 },
      { input: 'grid:0,6', action: 'Press', atMs: 160 },
      { input: 'grid:0,6', action: 'Release', atMs: 190 },
      { input: 'grid:0,1', action: 'Press', atMs: 220 },
      { input: 'grid:0,1', action: 'Release', atMs: 250 },
    ],
  })
  let nvsEntry = await waitForNvsRaw(socket, player1ColorKey, 'FF 00 66 00')
  assert(nvsEntry?.rawBytes === 'FF 00 66 00', 'gomoku.py did not persist player 1 color to NVS')

  nvsEntry = await waitForNvsRaw(socket, player2ColorKey, '40 40 FF 00')
  assert(nvsEntry?.rawBytes === '40 40 FF 00', 'gomoku.py did not persist player 2 color to NVS')

  led = await waitForLed(socket, 'grid:0,6', 'F25E1300')
  assert(led.color === 'F25E1300', `gomoku.py did not select winning length 3: ${led.color}`)

  const winningLengthKey = await rpcCall(socket, 'storage.nvs.computeHash', { text: 'Python Gomoku winning_length' })
  nvsEntry = await waitForNvsRaw(socket, winningLengthKey.hash, '03')
  assert(nvsEntry?.rawBytes === '03', 'gomoku.py did not persist winning_length to NVS')

  const firstPlayerKey = await rpcCall(socket, 'storage.nvs.computeHash', { text: 'Python Gomoku first_player' })
  nvsEntry = await waitForNvsRaw(socket, firstPlayerKey.hash, '02')
  assert(
    nvsEntry?.rawBytes === '02',
    `gomoku.py did not persist first_player to NVS: ${nvsEntry?.rawBytes ?? 'missing'}`,
  )

  await rpcCall(socket, 'input.execute', {
    events: [
      { input: 'function', action: 'Release' },
      { input: 'function', action: 'Press', atMs: 180 },
      { input: 'function', action: 'Release', atMs: 340 },
    ],
  })
  led = await waitForLedNot(socket, 'grid:7,3', 'FF000000')
  assert(led.color !== 'FF000000', 'gomoku.py did not return from settings after function-key release')

  await clickInput(socket, 'grid:0,0')
  await clickInput(socket, 'grid:0,7')
  await clickInput(socket, 'grid:1,0')
  await clickInput(socket, 'grid:1,7')
  await clickInput(socket, 'grid:2,0')

  let frame = await waitForLedFrame(socket, (candidate) => (
    Array.isArray(candidate.grid) &&
    candidate.grid.slice(0, 64).every((color) => color !== '00000000')
  ), 5_000)
  assert(
    frame.grid.slice(0, 64).every((color) => color !== '00000000'),
    'gomoku.py did not run win animation after three in a row',
  )
  await new Promise((resolve) => setTimeout(resolve, 2_600))

  await clickInput(socket, 'grid:4,4')
  frame = await waitForLedFrame(socket, (candidate) => (
    Array.isArray(candidate.grid) &&
    candidate.grid.slice(0, 64).every((color) => color === '00000000')
  ), 5_000)
  assert(
    frame.grid.slice(0, 64).every((color) => color === '00000000'),
    'gomoku.py did not reset after ended-state input',
  )

  await rpcCall(socket, 'input.execute', {
    events: [
      { input: 'function', action: 'Press' },
      { input: 'function', action: 'Release', atMs: 160 },
      { input: 'function', action: 'Press', atMs: 320 },
      { input: 'function', action: 'Press', atMs: 1_020 },
      { input: 'function', action: 'Release', atMs: 1_220 },
    ],
  })
  const finalStatus = await waitForPythonActive(socket, false, 5_000)
  assert(finalStatus?.active === false, 'gomoku.py did not exit on settings function-key hold')
  await new Promise((resolve) => setTimeout(resolve, 300))
  await rpcCall(socket, 'input.releaseAll').catch(() => {})
  await new Promise((resolve) => setTimeout(resolve, 100))
  console.log('[micropython-smoke] gomoku.py interaction ok')
}

async function smokeDiceInteraction(socket) {
  const modeHash = await setNvsRaw(socket, 'Python Dice mode', '00')
  const rollingRainbowHash = await setNvsRaw(socket, 'Python Dice rolling_rainbow', '00')
  const confirmedRainbowHash = await setNvsRaw(socket, 'Python Dice confirmed_rainbow', '00')
  const dotFacesHash = await setNvsRaw(socket, 'Python Dice dot_faces', '06')
  const numberFacesHash = await setNvsRaw(socket, 'Python Dice number_faces', '1e')
  const rollingUnderglowHash = await setNvsRaw(socket, 'Python Dice rolling_underglow', '04')
  const rollingPeriodHash = await setNvsRaw(socket, 'Python Dice rolling_period', '2c01')
  const confirmedUnderglowHash = await setNvsRaw(socket, 'Python Dice confirmed_underglow', '02')
  const confirmedPeriodHash = await setNvsRaw(socket, 'Python Dice confirmed_period', 'e8 03')

  await runExample(socket, 'dice')

  let frame = await waitForLedFrame(socket, (candidate) => (
    Array.isArray(candidate.grid) &&
    candidate.grid.some((color) => color !== '00000000')
  ), 3_000)
  assert(frame.grid.some((color) => color !== '00000000'), 'dice.py did not render a dice face')

  await rpcCall(socket, 'input.execute', {
    events: [
      { input: 'grid:0,0', action: 'Press' },
      { input: 'grid:0,0', action: 'Release' },
    ],
  })
  frame = await waitForLedFrame(socket, (candidate) => (
    Array.isArray(candidate.grid) &&
    candidate.grid.some((color) => color !== '00000000')
  ), 3_000)
  assert(frame.grid.some((color) => color !== '00000000'), 'dice.py did not keep rendering after roll input')

  await rpcCall(socket, 'input.execute', {
    events: [
      { input: 'function', action: 'Press' },
      { input: 'function', action: 'Press', atMs: 700 },
      { input: 'function', action: 'Release', atMs: 900 },
    ],
  })

  let led = await waitForLed(socket, 'grid:4,0', '33100000', 5_000)
  assert(led.color === '33100000', 'dice.py did not enter settings with number mode disabled')
  const rollingUnderglowLed = await waitForLedNot(socket, 'grid:0,7', '00000000', 3_000)
  assert(rollingUnderglowLed.color !== '00000000', 'dice.py settings rolling underglow button is not visible')
  const confirmedUnderglowLed = await waitForLedNot(socket, 'grid:7,7', '00000000', 3_000)
  assert(confirmedUnderglowLed.color !== '00000000', 'dice.py settings confirmed underglow button is not visible')

  await rpcCall(socket, 'input.execute', {
    events: [
      { input: 'grid:4,0', action: 'Press' },
      { input: 'grid:4,0', action: 'Release' },
    ],
  })
  led = await waitForLed(socket, 'grid:4,0', 'FF500000')
  assert(led.color === 'FF500000', 'dice.py did not enable number mode in settings')

  const nvsEntry = await waitForNvsRaw(socket, modeHash, '01')
  assert(nvsEntry?.rawBytes === '01', 'dice.py did not persist mode to NVS')

  await rpcCall(socket, 'input.execute', {
    events: [
      { input: 'grid:0,0', action: 'Press' },
      { input: 'grid:0,0', action: 'Release' },
    ],
  })
  let settingEntry = await waitForNvsRaw(socket, rollingRainbowHash, '01')
  assert(settingEntry?.rawBytes === '01', 'dice.py did not persist rolling rainbow toggle to NVS')

  await rpcCall(socket, 'input.execute', {
    events: [
      { input: 'grid:7,0', action: 'Press' },
      { input: 'grid:7,0', action: 'Release' },
    ],
  })
  settingEntry = await waitForNvsRaw(socket, confirmedRainbowHash, '01')
  assert(settingEntry?.rawBytes === '01', 'dice.py did not persist confirmed rainbow toggle to NVS')
  await new Promise((resolve) => setTimeout(resolve, 300))

  await rpcCall(socket, 'input.execute', {
    events: [
      { input: 'grid:0,7', action: 'Press' },
      { input: 'grid:0,7', action: 'Release', atMs: 200 },
    ],
  })
  await new Promise((resolve) => setTimeout(resolve, 500))

  await rpcCall(socket, 'input.execute', {
    events: [
      { input: 'grid:3,0', action: 'Press' },
      { input: 'grid:3,0', action: 'Release', atMs: 120 },
      { input: 'grid:7,6', action: 'Press', atMs: 260 },
      { input: 'grid:7,6', action: 'Release', atMs: 380 },
      { input: 'function', action: 'Press', atMs: 520 },
      { input: 'function', action: 'Release', atMs: 640 },
    ],
  })
  settingEntry = await waitForNvsRaw(socket, rollingUnderglowHash, '02')
  assert(
    settingEntry?.rawBytes === '02',
    `dice.py did not persist rolling underglow effect to NVS: ${settingEntry?.rawBytes ?? 'missing'}`,
  )
  settingEntry = await waitForNvsRaw(socket, rollingPeriodHash, '84 03')
  const underglowOutput = outputText(await rpcCall(socket, 'python.getOutput', { last: 80 }))
  assert(!underglowOutput.includes('Traceback'), `dice.py emitted traceback during underglow effect menu: ${underglowOutput}`)
  assert(
    settingEntry?.rawBytes === '84 03',
    `dice.py did not persist rolling underglow speed to NVS: ${settingEntry?.rawBytes ?? 'missing'}`,
  )

  await rpcCall(socket, 'input.execute', {
    events: [
      { input: 'grid:7,7', action: 'Press' },
      { input: 'grid:7,7', action: 'Release', atMs: 200 },
    ],
  })
  await new Promise((resolve) => setTimeout(resolve, 500))

  await rpcCall(socket, 'input.execute', {
    events: [
      { input: 'grid:4,0', action: 'Press' },
      { input: 'grid:4,0', action: 'Release', atMs: 120 },
      { input: 'grid:6,6', action: 'Press', atMs: 260 },
      { input: 'grid:6,6', action: 'Release', atMs: 380 },
      { input: 'function', action: 'Press', atMs: 520 },
      { input: 'function', action: 'Release', atMs: 640 },
    ],
  })
  settingEntry = await waitForNvsRaw(socket, confirmedUnderglowHash, '03')
  assert(
    settingEntry?.rawBytes === '03',
    `dice.py did not persist confirmed underglow effect to NVS: ${settingEntry?.rawBytes ?? 'missing'}`,
  )
  settingEntry = await waitForNvsRaw(socket, confirmedPeriodHash, '20 03')
  const confirmedUnderglowOutput = outputText(await rpcCall(socket, 'python.getOutput', { last: 80 }))
  assert(!confirmedUnderglowOutput.includes('Traceback'), `dice.py emitted traceback during confirmed underglow effect menu: ${confirmedUnderglowOutput}`)
  assert(
    settingEntry?.rawBytes === '20 03',
    `dice.py did not persist confirmed underglow speed to NVS: ${settingEntry?.rawBytes ?? 'missing'}`,
  )

  await rpcCall(socket, 'input.execute', {
    events: [
      { input: 'grid:3,0', action: 'Press' },
      { input: 'grid:3,0', action: 'Release' },
    ],
  })
  settingEntry = await waitForNvsRaw(socket, modeHash, '00')
  assert(settingEntry?.rawBytes === '00', 'dice.py did not persist dot mode to NVS')

  await rpcCall(socket, 'input.execute', {
    events: [
      { input: 'grid:4,0', action: 'Press' },
      { input: 'grid:4,0', action: 'Release' },
    ],
  })
  settingEntry = await waitForNvsRaw(socket, modeHash, '01')
  assert(settingEntry?.rawBytes === '01', 'dice.py did not persist number mode before number faces selector')

  await rpcCall(socket, 'input.execute', {
    events: [
      { input: 'grid:2,7', action: 'Press' },
      { input: 'grid:2,7', action: 'Release' },
    ],
  })
  await new Promise((resolve) => setTimeout(resolve, 300))

  await rpcCall(socket, 'input.execute', {
    events: [
      { input: 'grid:3,7', action: 'Press' },
      { input: 'grid:3,7', action: 'Release' },
      { input: 'function', action: 'Press' },
      { input: 'function', action: 'Release' },
    ],
  })
  settingEntry = await waitForNvsRaw(socket, numberFacesHash, '1D')
  const numberSelectorOutput = outputText(await rpcCall(socket, 'python.getOutput', { last: 80 }))
  assert(!numberSelectorOutput.includes('Traceback'), `dice.py emitted traceback during number faces selector: ${numberSelectorOutput}`)
  assert(
    settingEntry?.rawBytes === '1D',
    `dice.py did not persist number faces selector value to NVS: ${settingEntry?.rawBytes ?? 'missing'}`,
  )

  await rpcCall(socket, 'input.execute', {
    events: [
      { input: 'grid:3,0', action: 'Press' },
      { input: 'grid:3,0', action: 'Release' },
    ],
  })
  settingEntry = await waitForNvsRaw(socket, modeHash, '00')
  assert(settingEntry?.rawBytes === '00', 'dice.py did not return to dot mode before dot faces selector')

  await rpcCall(socket, 'input.execute', {
    events: [
      { input: 'grid:2,7', action: 'Press' },
      { input: 'grid:2,7', action: 'Release' },
    ],
  })
  led = await waitForLed(socket, 'grid:4,7', '00FFFF00', 5_000)
  assert(led.color === '00FFFF00', 'dice.py did not open dot faces selector')
  await new Promise((resolve) => setTimeout(resolve, 300))

  await rpcCall(socket, 'input.execute', {
    events: [
      { input: 'grid:6,7', action: 'Press' },
    ],
  })
  await new Promise((resolve) => setTimeout(resolve, 120))
  await rpcCall(socket, 'input.execute', {
    events: [
      { input: 'grid:6,7', action: 'Release' },
    ],
  })
  led = await waitForLed(socket, 'grid:6,7', '00FFFF00', 3_000)
  assert(led.color === '00FFFF00', 'dice.py dot faces selector did not update selected value')
  await rpcCall(socket, 'input.execute', {
    events: [
      { input: 'function', action: 'Press' },
      { input: 'function', action: 'Release' },
    ],
  })
  led = await waitForLed(socket, 'grid:3,0', 'FF00FF00', 3_000)
  assert(led.color === 'FF00FF00', 'dice.py dot faces selector did not return to settings UI')
  settingEntry = await waitForNvsRaw(socket, dotFacesHash, '08')
  const diceOutput = outputText(await rpcCall(socket, 'python.getOutput', { last: 80 }))
  assert(!diceOutput.includes('Traceback'), `dice.py emitted traceback during dot faces selector: ${diceOutput}`)
  assert(
    settingEntry?.rawBytes === '08',
    `dice.py did not persist dot faces selector value to NVS: ${settingEntry?.rawBytes ?? 'missing'}`,
  )

  await holdFunctionKeyToExitExample(socket, 'dice.py')
  console.log('[micropython-smoke] dice.py interaction ok')
}

async function smokeMultiFileImport(socket) {
  await stopPython(socket)

  const files = [
    {
      name: 'helper.py',
      text: [
        'VALUE = 37',
        '',
        'def label():',
        '    return "helper-ok"',
        '',
      ].join('\n'),
    },
    {
      name: 'main.py',
      text: [
        'import helper',
        '',
        'with open("multi_file_smoke.txt", "w") as f:',
        '    f.write(helper.label())',
        '',
        'with open("multi_file_smoke.txt", "r") as f:',
        '    text = f.read()',
        '',
        'print("multi_file_import", helper.VALUE, text)',
        '',
        'def loop():',
        '    pass',
        '',
      ].join('\n'),
    },
  ]

  const staged = await rpcCall(socket, 'python.stageFiles', { files, entry: 'main.py' })
  assert(staged.ok && staged.count === files.length, 'multi-file stage failed')
  const run = await runPythonStaged(socket)
  assert(run.ok, 'multi-file runStaged failed')

  await waitForOutput(socket, (text) => text.includes('multi_file_import 37 helper-ok'))
  await stopPython(socket)
  console.log('[micropython-smoke] multi-file import/open ok')
}

async function runSelectedExampleInteraction(socket, name) {
  if (name === 'pixel_art') await smokePixelArtInteraction(socket)
  else if (name === 'same_game') await smokeSameGameInteraction(socket)
  else if (name === 'gomoku') await smokeGomokuInteraction(socket)
  else if (name === 'dice') await smokeDiceInteraction(socket)
  else throw new Error(`No interaction smoke registered for ${name}`)
}

async function run() {
  const { wsUrl, suites, examples } = parseArgs()
  console.log(`[micropython-smoke] connecting to ${wsUrl}`)
  console.log(`[micropython-smoke] suites: ${suites.join(', ')}`)
  if (suites.includes('examples')) console.log(`[micropython-smoke] examples: ${examples.join(', ')}`)

  const socket = await connect(wsUrl)
  try {
    const status = await rpcCall(socket, 'python.status')
    assert(status.available, 'Python app is not available')

    if (suites.includes('core')) {
      await smokeRunText(socket)
      await smokeRepl(socket)
      await smokeSubscribe(socket)
      await smokeInput(socket)
      await smokeUsbCdc(socket)
      await smokeMidi(socket)
      await smokeHidRaw(socket)
      await smokeExceptionReporting(socket)
      await smokeApiIntrospection(socket)
    }

    if (suites.includes('filesystem')) {
      await smokeMultiFileImport(socket)
    }

    if (suites.includes('ui')) {
      await smokeUiInteraction(socket)
      await smokeUiCallbackException(socket)
      await smokeUiTextScrollHold(socket)
      await smokeUiSelectorTextScrollHold(socket)
      await smokeUiNumberTextScrollHold(socket)
      await smokeUiFunctionHoldExit(socket)
      await smokeUiColorPicker(socket)
      await smokeUiNumberSelector(socket)
    }

    if (suites.includes('lifecycle')) {
      await smokeLifecycleStress(socket)
    }

    if (suites.includes('examples')) {
      for (const name of examples) {
        await runSelectedExampleInteraction(socket, name)
      }

      for (const name of examples) {
        await smokeExample(socket, name)
      }
    }

    console.log('[micropython-smoke] all checks passed')
  } catch (error) {
    const diagnostics = await collectDiagnostics(socket, {
      wsUrl,
      suites,
      examples: suites.includes('examples') ? examples : [],
      error: String(error?.message ?? error),
    })
    console.error(`[micropython-smoke] diagnostics:\n${formatDiagnostics(diagnostics)}`)
    throw error
  } finally {
    socket.close()
  }
}

run().catch((error) => {
  console.error(`[micropython-smoke] failed: ${error.message}`)
  process.exit(1)
})
