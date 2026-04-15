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

function optionalDeviceGroup(prefix) {
  const devices = {}

  for (const [deviceName, envName] of [
    ['Mystrix1', `${prefix}_MYSTRIX1_FILES`],
    ['Mystrix2', `${prefix}_MYSTRIX2_FILES`],
    ['MystrixSim', `${prefix}_MYSTRIXSIM_FILES`]
  ]) {
    const files = optionalFileList(envName)
    if (files.length > 0) {
      devices[deviceName] = { files }
    }
  }

  return devices
}

const release = {}
for (const [channelName, prefix] of [
  ['release', 'INDEX_RELEASE'],
  ['rc', 'INDEX_RC'],
  ['beta', 'INDEX_BETA'],
  ['nightly', 'INDEX_NIGHTLY'],
  ['development', 'INDEX_DEVELOPMENT']
]) {
  const devices = optionalDeviceGroup(prefix)
  if (Object.keys(devices).length > 0) {
    release[channelName] = devices
  }
}

const index = {
  schemaVersion: 2,
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
  release,
}

const outputPath = path.resolve(requireEnv('INDEX_OUTPUT'))
fs.mkdirSync(path.dirname(outputPath), { recursive: true })
fs.writeFileSync(outputPath, `${JSON.stringify(index, null, 2)}\n`, 'utf8')