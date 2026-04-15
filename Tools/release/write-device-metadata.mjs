#!/usr/bin/env node

import fs from 'node:fs'
import path from 'node:path'

function requireEnv(name) {
  const value = process.env[name]
  if (!value) {
    throw new Error(`Missing required environment variable: ${name}`)
  }
  return value
}

function optionalFileList(name) {
  const value = process.env[name]
  if (!value) {
    return []
  }

  return value
    .split(/\r?\n/)
    .map((entry) => entry.trim())
    .filter(Boolean)
}

const devices = {}
for (const [deviceName, envName] of [
  ['Mystrix1', 'INDEX_MYSTRIX1_FILES'],
  ['Mystrix2', 'INDEX_MYSTRIX2_FILES'],
  ['MystrixSim', 'INDEX_MYSTRIXSIM_FILES']
]) {
  const files = optionalFileList(envName)
  if (files.length > 0) {
    devices[deviceName] = { files }
  }
}

const index = {
  tag: requireEnv('INDEX_TAG'),
  name: requireEnv('INDEX_NAME'),
  channel: requireEnv('INDEX_CHANNEL'),
  channelSlug: requireEnv('INDEX_CHANNEL_SLUG'),
  version: requireEnv('INDEX_VERSION'),
  displayVersion: requireEnv('INDEX_DISPLAY_VERSION'),
  date: requireEnv('INDEX_DATE'),
  generatedAt: requireEnv('INDEX_GENERATED_AT'),
  commit: requireEnv('INDEX_COMMIT'),
  prerelease: /^true$/i.test(requireEnv('INDEX_PRERELEASE')),
  devices,
}

const outputPath = path.resolve(requireEnv('INDEX_OUTPUT'))
fs.mkdirSync(path.dirname(outputPath), { recursive: true })
fs.writeFileSync(outputPath, `${JSON.stringify(index, null, 2)}\n`, 'utf8')