#!/usr/bin/env node

import fs from 'node:fs'
import path from 'node:path'
import JSZip from 'jszip'

const [jsPath, wasmPath, outputPath] = process.argv.slice(2)

if (!jsPath || !wasmPath || !outputPath) {
  throw new Error('Usage: node package-runtime.mjs <MatrixOSHost.js> <MatrixOSHost.wasm> <output.mspkg>')
}

const zip = new JSZip()
zip.file('MatrixOSHost.js', fs.readFileSync(jsPath))
zip.file('MatrixOSHost.wasm', fs.readFileSync(wasmPath))

const buffer = await zip.generateAsync({
  type: 'nodebuffer',
  compression: 'DEFLATE',
  compressionOptions: { level: 9 },
})

fs.mkdirSync(path.dirname(outputPath), { recursive: true })
fs.writeFileSync(outputPath, buffer)
