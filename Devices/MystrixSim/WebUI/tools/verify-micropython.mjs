#!/usr/bin/env node
/**
 * MatrixOS MicroPython Phase 2 verification runner.
 *
 * Default:
 *   static checks + MatrixOSHost build + wasm validation + runtime package +
 *   WebUI production build.
 *
 * Optional:
 *   --smoke ws://localhost:4012  Run RPC smoke against an existing browser tab.
 *   --smoke-dev                 Start WebUI dev server + Chrome, then run smoke.
 */

import { spawn } from 'node:child_process'
import { existsSync, mkdirSync, readFileSync, readdirSync } from 'node:fs'
import { createServer } from 'node:net'
import { tmpdir } from 'node:os'
import { dirname, join, resolve } from 'node:path'
import { fileURLToPath } from 'node:url'

const toolDir = dirname(fileURLToPath(import.meta.url))
const webUiDir = resolve(toolDir, '..')
const repoRoot = resolve(webUiDir, '..', '..', '..')
const matrixosUsermodDir = join(repoRoot, 'Applications', 'Python', 'MicroPythonPort', 'usermod', 'matrixos')
const buildDir = join(repoRoot, 'build', 'MystrixSim')
const hostDir = join(buildDir, 'Devices', 'MystrixSim')
const hostJs = join(hostDir, 'MatrixOSHost.js')
const hostWasm = join(hostDir, 'MatrixOSHost.wasm')
const packagedRuntime = join(webUiDir, 'public', 'MatrixOS.msfw')

const nodeCmd = process.execPath
const npmCmd = process.platform === 'win32' ? 'npm.cmd' : 'npm'
const pythonCmd = process.env.PYTHON || 'python'
const exampleSmokeApps = [
  { example: 'pixel_art', scriptName: 'pixel-art-smoke.mjs', packageScript: 'smoke:micropython:pixel-art' },
  { example: 'same_game', scriptName: 'same-game-smoke.mjs', packageScript: 'smoke:micropython:same-game' },
  { example: 'gomoku', scriptName: 'gomoku-smoke.mjs', packageScript: 'smoke:micropython:gomoku' },
  { example: 'dice', scriptName: 'dice-smoke.mjs', packageScript: 'smoke:micropython:dice' },
]
const smokeScripts = [
  'micropython-smoke.mjs',
  ...exampleSmokeApps.map((app) => app.scriptName),
]
const requiredPackageScripts = {
  'smoke:micropython': 'node tools/micropython-smoke.mjs',
  ...Object.fromEntries(exampleSmokeApps.map((app) => [app.packageScript, `node tools/${app.scriptName}`])),
  'verify:micropython': 'node tools/verify-micropython.mjs',
  'verify:micropython:smoke': 'node tools/verify-micropython.mjs --smoke-dev',
}

function needsShell(command) {
  return process.platform === 'win32' && /\.(cmd|bat)$/i.test(command)
}

function quoteWindowsArg(arg) {
  const text = String(arg)
  if (!/[ \t"&|<>^]/.test(text)) return text
  return `"${text.replace(/"/g, '\\"')}"`
}

function spawnSpec(command, args) {
  if (!needsShell(command)) {
    return { command, args, shell: false }
  }

  const line = [command, ...args].map(quoteWindowsArg).join(' ')
  return {
    command: process.env.ComSpec || 'cmd.exe',
    args: ['/d', '/s', '/c', line],
    shell: false,
  }
}

function parseArgs() {
  const args = process.argv.slice(2)
  const options = {
    smokeWs: '',
    smokeDev: false,
    skipBuild: false,
    skipWebBuild: false,
    skipStatic: false,
    suites: [],
    examples: [],
    rpcPort: process.env.MATRIXOS_RPC_PORT || '',
    webPort: '',
    rpcPortExplicit: Boolean(process.env.MATRIXOS_RPC_PORT),
    webPortExplicit: false,
  }

  for (let i = 0; i < args.length; i++) {
    const arg = args[i]
    if (arg === '--smoke') options.smokeWs = args[++i] || ''
    else if (arg === '--smoke-dev') options.smokeDev = true
    else if (arg === '--skip-build') options.skipBuild = true
    else if (arg === '--skip-web-build') options.skipWebBuild = true
    else if (arg === '--skip-static') options.skipStatic = true
    else if (arg === '--suite') options.suites.push(...(args[++i] || '').split(',').map((item) => item.trim()).filter(Boolean))
    else if (arg === '--example') options.examples.push(...(args[++i] || '').split(',').map((item) => item.trim()).filter(Boolean))
    else if (arg === '--rpc-port') {
      options.rpcPort = args[++i] || options.rpcPort
      options.rpcPortExplicit = true
    }
    else if (arg === '--web-port') {
      options.webPort = args[++i] || options.webPort
      options.webPortExplicit = true
    }
    else if (arg === '--help' || arg === '-h') {
      printUsage()
      process.exit(0)
    } else {
      throw new Error(`Unknown argument: ${arg}`)
    }
  }

  return options
}

function printUsage() {
  console.log([
    'Usage:',
    '  npm --prefix Devices/MystrixSim/WebUI run verify:micropython',
    '  npm --prefix Devices/MystrixSim/WebUI run verify:micropython -- --smoke ws://localhost:4012',
    '  npm --prefix Devices/MystrixSim/WebUI run verify:micropython:smoke',
    '',
    'Options:',
    '  --smoke <ws-url>     Run WebUI RPC smoke against an existing runtime tab.',
    '  --smoke-dev          Start dev server + Chrome and run RPC smoke.',
    '  --suite <name>       Forward smoke suite filter: core, filesystem, ui, lifecycle, examples, or all.',
    '  --example <name>     Forward example filter for examples suite: pixel_art, same_game, gomoku, dice, or all.',
    '  --rpc-port <port>    WebSocket RPC port for --smoke-dev. Default: auto.',
    '  --web-port <port>    Vite dev server port for --smoke-dev. Default: auto.',
    '  --skip-build         Skip MatrixOSHost build/wasm/package steps.',
    '  --skip-web-build     Skip WebUI production build.',
    '  --skip-static        Skip static JS/Python/qstr checks.',
  ].join('\n'))
}

function logStep(message) {
  console.log(`\n[verify-micropython] ${message}`)
}

function run(command, args, options = {}) {
  const printable = [command, ...args].join(' ')
  console.log(`[verify-micropython] $ ${printable}`)
  const spec = spawnSpec(command, args)

  return new Promise((resolve, reject) => {
    const child = spawn(spec.command, spec.args, {
      cwd: repoRoot,
      env: { ...process.env, ...(options.env || {}) },
      stdio: 'inherit',
      shell: spec.shell,
    })

    child.on('error', reject)
    child.on('exit', (code, signal) => {
      if (code === 0) resolve()
      else reject(new Error(`${printable} failed with ${signal || `exit code ${code}`}`))
    })
  })
}

function sleep(ms) {
  return new Promise((resolve) => setTimeout(resolve, ms))
}

function findAvailablePort() {
  return new Promise((resolve, reject) => {
    const server = createServer()
    server.unref()
    server.on('error', reject)
    server.listen(0, '127.0.0.1', () => {
      const address = server.address()
      const port = typeof address === 'object' && address ? address.port : 0
      server.close(() => resolve(String(port)))
    })
  })
}

async function prepareSmokeDevPorts(options) {
  if (!options.smokeDev) return
  if (!options.rpcPortExplicit) options.rpcPort = await findAvailablePort()
  if (!options.webPortExplicit) options.webPort = await findAvailablePort()
  options.smokeWs = `ws://localhost:${options.rpcPort}`
}

function checkQstrOrder() {
  const qstrPath = join(repoRoot, 'Applications', 'Python', 'MicroPythonEmbed', 'genhdr', 'qstrdefs.generated.h')
  const lines = readFileSync(qstrPath, 'utf8').split(/\r?\n/)
  const items = []
  const pattern = /^QDEF1\(MP_QSTR_[^,]+, [^,]+, [^,]+, "(.*)"\)/

  for (let i = 0; i < lines.length; i++) {
    const match = pattern.exec(lines[i].trim())
    if (match) items.push({ line: i + 1, qstr: match[1] })
  }

  for (let i = 1; i < items.length; i++) {
    if (items[i - 1].qstr > items[i].qstr) {
      throw new Error(
        `qstr order inversion: line ${items[i - 1].line} ${items[i - 1].qstr} > line ${items[i].line} ${items[i].qstr}`,
      )
    }
  }

  console.log('[verify-micropython] qstr order ok')
}

function checkPackageScripts() {
  const packagePath = join(webUiDir, 'package.json')
  const packageJson = JSON.parse(readFileSync(packagePath, 'utf8'))
  const scripts = packageJson.scripts || {}

  for (const [name, command] of Object.entries(requiredPackageScripts)) {
    if (scripts[name] !== command) {
      throw new Error(`package.json script ${name} must be "${command}", got "${scripts[name] ?? '<missing>'}"`)
    }
  }

  for (const script of smokeScripts) {
    const scriptPath = join(webUiDir, 'tools', script)
    if (!existsSync(scriptPath)) {
      throw new Error(`package.json references missing smoke script: ${script}`)
    }
  }

  console.log('[verify-micropython] package scripts ok')
}

function parseMicropythonSmokeExamples() {
  const smokePath = join(webUiDir, 'tools', 'micropython-smoke.mjs')
  const text = readFileSync(smokePath, 'utf8')
  const match = /const EXAMPLES = \[([^\]]*)\]/m.exec(text)
  if (!match) {
    throw new Error('micropython-smoke.mjs is missing EXAMPLES list')
  }

  return Array.from(match[1].matchAll(/'([^']+)'/g), (item) => item[1].replace(/\.py$/, ''))
}

function checkExampleSmokeEntrypoints() {
  const smokeExamples = parseMicropythonSmokeExamples()
  const manifestExamples = exampleSmokeApps.map((app) => app.example)
  if (smokeExamples.join(',') !== manifestExamples.join(',')) {
    throw new Error(
      `micropython-smoke.mjs EXAMPLES must match app smoke manifest: ${manifestExamples.join(', ')}, got ${smokeExamples.join(', ')}`,
    )
  }

  for (const app of exampleSmokeApps) {
    const wrapperPath = join(webUiDir, 'tools', app.scriptName)
    const wrapperText = readFileSync(wrapperPath, 'utf8')
    if (!wrapperText.includes("'--suite', 'examples'") || !wrapperText.includes(`'--example', '${app.example}'`)) {
      throw new Error(`${app.scriptName} must run micropython-smoke.mjs --suite examples --example ${app.example}`)
    }
  }

  console.log('[verify-micropython] example smoke entrypoints ok')
}

function checkMicropythonUsermodManifest() {
  const makefilePath = join(matrixosUsermodDir, 'micropython.mk')
  const makefileText = readFileSync(makefilePath, 'utf8')
  const makefileSources = Array.from(
    makefileText.matchAll(/\$\(MATRIXOS_MOD_DIR\)\/(matrixos_[^\s\\]+\.cpp)/g),
    (match) => match[1],
  ).sort()
  const cmakePath = join(repoRoot, 'Applications', 'Python', 'CMakeLists.txt')
  const cmakeText = readFileSync(cmakePath, 'utf8')
  const cmakeSources = Array.from(
    cmakeText.matchAll(/\$\{MICROPYTHON_PORT_DIR\}\/usermod\/matrixos\/(matrixos_[^\s]+\.cpp)/g),
    (match) => match[1],
  ).sort()
  const directorySources = readdirSync(matrixosUsermodDir)
    .filter((name) => /^matrixos_.*\.cpp$/.test(name))
    .sort()

  const missingFromMakefile = directorySources.filter((name) => !makefileSources.includes(name))
  const missingFromCmake = directorySources.filter((name) => !cmakeSources.includes(name))
  const missingFromDirectory = [...new Set([...makefileSources, ...cmakeSources])]
    .filter((name) => !directorySources.includes(name))
  if (missingFromMakefile.length > 0 || missingFromCmake.length > 0 || missingFromDirectory.length > 0) {
    throw new Error(
      [
        'MicroPython usermod source manifests do not match matrixos usermod sources.',
        missingFromMakefile.length > 0 ? `Missing from makefile: ${missingFromMakefile.join(', ')}` : '',
        missingFromCmake.length > 0 ? `Missing from CMakeLists.txt: ${missingFromCmake.join(', ')}` : '',
        missingFromDirectory.length > 0 ? `Missing from directory: ${missingFromDirectory.join(', ')}` : '',
      ].filter(Boolean).join(' '),
    )
  }

  if (existsSync(join(matrixosUsermodDir, 'modmatrixos.cpp'))) {
    throw new Error('modmatrixos.cpp must not be reintroduced; use matrixos_module.cpp plus subsystem files')
  }

  console.log('[verify-micropython] MicroPython usermod manifest ok')
}

async function runStaticChecks() {
  logStep('running static checks')
  checkPackageScripts()
  checkExampleSmokeEntrypoints()
  checkMicropythonUsermodManifest()
  for (const script of [...smokeScripts, 'verify-micropython.mjs']) {
    await run(nodeCmd, ['--check', join(webUiDir, 'tools', script)])
  }
  await run(nodeCmd, ['--check', join(webUiDir, 'src', 'stores', 'rpc.js')])
  await run(nodeCmd, ['--check', join(webUiDir, 'src', 'stores', 'storage.js')])
  await run(nodeCmd, ['--check', join(webUiDir, 'src', 'handles', 'python.js')])
  await run(nodeCmd, ['--check', join(webUiDir, 'vite-plugin-rpc-server.js')])
  await run(pythonCmd, [
    '-m',
    'py_compile',
    join(repoRoot, 'Applications', 'Python', 'examples', 'api_introspection', 'main.py'),
    join(repoRoot, 'Applications', 'Python', 'examples', 'pixel_art', 'main.py'),
    join(repoRoot, 'Applications', 'Python', 'examples', 'same_game', 'main.py'),
    join(repoRoot, 'Applications', 'Python', 'examples', 'gomoku', 'main.py'),
    join(repoRoot, 'Applications', 'Python', 'examples', 'dice', 'main.py'),
  ])
  await run(pythonCmd, [join(repoRoot, 'Applications', 'Python', 'tools', 'check_micropython_api_surface.py')])
  checkQstrOrder()
}

async function runBuildAndPackage() {
  logStep('building MatrixOSHost')
  const env = { ...process.env }
  if (!env.EM_CACHE) {
    env.EM_CACHE = join(repoRoot, 'build', 'emscripten-cache')
    mkdirSync(env.EM_CACHE, { recursive: true })
  }

  await run('cmake', ['--build', buildDir, '--target', 'MatrixOSHost', '--parallel'], { env })

  if (!existsSync(hostJs) || !existsSync(hostWasm)) {
    throw new Error(`MatrixOSHost build outputs are missing under ${hostDir}`)
  }

  logStep('validating wasm')
  await run(nodeCmd, [join(repoRoot, 'Devices', 'MystrixSim', 'tools', 'validate-runtime-wasm.mjs'), hostWasm])

  logStep('packaging WebUI runtime')
  await run(nodeCmd, [join(webUiDir, 'tools', 'package-runtime.mjs'), hostJs, hostWasm, packagedRuntime])
}

async function runWebBuild() {
  logStep('building WebUI')
  await run(npmCmd, ['--prefix', webUiDir, 'run', 'build'])
}

function chromeCandidates() {
  if (process.env.CHROME_PATH) return [process.env.CHROME_PATH]
  if (process.platform === 'win32') {
    return [
      'C:\\Program Files\\Google\\Chrome\\Application\\chrome.exe',
      'C:\\Program Files (x86)\\Google\\Chrome\\Application\\chrome.exe',
    ]
  }
  if (process.platform === 'darwin') {
    return ['/Applications/Google Chrome.app/Contents/MacOS/Google Chrome']
  }
  return ['google-chrome', 'chromium', 'chromium-browser']
}

function findChrome() {
  for (const candidate of chromeCandidates()) {
    if (candidate.includes('\\') || candidate.startsWith('/')) {
      if (existsSync(candidate)) return candidate
    } else {
      return candidate
    }
  }
  throw new Error('Chrome not found. Set CHROME_PATH or run --smoke against an existing browser tab.')
}

async function withDevRuntime(options, callback) {
  logStep('starting WebUI dev server')
  const env = {
    ...process.env,
    MATRIXOS_RPC_PORT: options.rpcPort,
    VITE_MATRIXOS_RPC_PORT: options.rpcPort,
  }
  const devSpec = spawnSpec(npmCmd, ['--prefix', webUiDir, 'run', 'dev', '--', '--host', '127.0.0.1', '--port', options.webPort])
  const dev = spawn(devSpec.command, devSpec.args, {
    cwd: repoRoot,
    env,
    stdio: 'ignore',
    shell: devSpec.shell,
  })

  let chrome = null
  const cleanup = () => {
    if (chrome && !chrome.killed) chrome.kill()
    if (dev && !dev.killed) dev.kill()
  }
  process.once('exit', cleanup)

  try {
    await sleep(8_000)
    const profile = join(tmpdir(), `matrixos-smoke-chrome-${Date.now()}`)
    mkdirSync(profile, { recursive: true })
    const url = `http://127.0.0.1:${options.webPort}/?v=${Date.now()}`
    const chromePath = findChrome()
    logStep(`starting Chrome at ${url}`)
    chrome = spawn(chromePath, [
      '--new-window',
      '--disable-cache',
      '--disable-background-timer-throttling',
      '--disable-backgrounding-occluded-windows',
      '--disable-renderer-backgrounding',
      '--disable-features=CalculateNativeWinOcclusion',
      '--no-first-run',
      '--no-default-browser-check',
      `--user-data-dir=${profile}`,
      url,
    ], {
      cwd: repoRoot,
      stdio: 'ignore',
      shell: false,
    })
    await sleep(20_000)
    await callback()
  } finally {
    cleanup()
    process.removeListener('exit', cleanup)
  }
}

async function runSmoke(wsUrl, suites = [], examples = []) {
  logStep(`running RPC smoke: ${wsUrl}`)
  const suiteArgs = suites.flatMap((suite) => ['--suite', suite])
  const exampleArgs = examples.flatMap((example) => ['--example', example])
  await run(npmCmd, ['--prefix', webUiDir, 'run', 'smoke:micropython', '--', '--ws', wsUrl, ...suiteArgs, ...exampleArgs])
}

async function main() {
  const options = parseArgs()
  await prepareSmokeDevPorts(options)

  if (!options.skipStatic) await runStaticChecks()
  if (!options.skipBuild) await runBuildAndPackage()
  if (!options.skipWebBuild) await runWebBuild()

  if (options.smokeDev) {
    await withDevRuntime(options, () => runSmoke(options.smokeWs, options.suites, options.examples))
  } else if (options.smokeWs) {
    await runSmoke(options.smokeWs, options.suites, options.examples)
  }

  console.log('\n[verify-micropython] all requested checks passed')
}

main().catch((error) => {
  console.error(`\n[verify-micropython] failed: ${error instanceof Error ? error.message : String(error)}`)
  process.exit(1)
})
