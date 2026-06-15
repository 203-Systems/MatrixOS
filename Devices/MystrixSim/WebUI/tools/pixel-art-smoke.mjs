#!/usr/bin/env node
import { spawnSync } from 'node:child_process'
import { dirname, join } from 'node:path'
import { fileURLToPath } from 'node:url'

const toolDir = dirname(fileURLToPath(import.meta.url))
const smokeScript = join(toolDir, 'micropython-smoke.mjs')
const result = spawnSync(process.execPath, [smokeScript, '--suite', 'examples', '--example', 'pixel_art', ...process.argv.slice(2)], {
  stdio: 'inherit',
})

process.exit(result.status ?? 1)
