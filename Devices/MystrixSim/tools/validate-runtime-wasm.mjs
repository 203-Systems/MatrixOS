#!/usr/bin/env node

import fs from 'node:fs/promises'
import process from 'node:process'

const wasmPath = process.argv[2]

if (!wasmPath) {
  throw new Error('Usage: node validate-runtime-wasm.mjs <MatrixOSHost.wasm>')
}

const wasmBytes = await fs.readFile(wasmPath)

if (!WebAssembly.validate(wasmBytes)) {
  throw new Error(`Invalid WebAssembly module: ${wasmPath}`)
}

try {
  await WebAssembly.compile(wasmBytes)
} catch (error) {
  throw new Error(`Failed to compile ${wasmPath}: ${error instanceof Error ? error.message : String(error)}`)
}

console.log(`[MystrixSim] Verified wasm: ${wasmPath}`)