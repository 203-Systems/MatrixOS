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

function optionalAsset(nameEnv, shaEnv, urlEnv) {
  const name = process.env[nameEnv]
  if (!name) {
    return null
  }

  const asset = {
    name,
    sha256: process.env[shaEnv] || ''
  }

  const url = process.env[urlEnv]
  if (url) {
    asset.url = url
  }

  return asset
}

const assets = {}
for (const [assetKey, nameEnv, shaEnv, urlEnv] of [
  ['firmware', 'METADATA_FIRMWARE_NAME', 'METADATA_FIRMWARE_SHA', 'METADATA_FIRMWARE_URL'],
  ['loader', 'METADATA_LOADER_NAME', 'METADATA_LOADER_SHA', 'METADATA_LOADER_URL'],
  ['wasm', 'METADATA_WASM_NAME', 'METADATA_WASM_SHA', 'METADATA_WASM_URL'],
  ['bundle', 'METADATA_BUNDLE_NAME', 'METADATA_BUNDLE_SHA', 'METADATA_BUNDLE_URL']
]) {
  const asset = optionalAsset(nameEnv, shaEnv, urlEnv)
  if (asset) {
    assets[assetKey] = asset
  }
}

if (Object.keys(assets).length === 0) {
  throw new Error('At least one metadata asset must be provided')
}

const metadata = {
  tag: requireEnv('METADATA_TAG'),
  name: requireEnv('METADATA_NAME'),
  channel: requireEnv('METADATA_CHANNEL'),
  channelSlug: requireEnv('METADATA_CHANNEL_SLUG'),
  version: requireEnv('METADATA_VERSION'),
  displayVersion: requireEnv('METADATA_DISPLAY_VERSION'),
  date: requireEnv('METADATA_DATE'),
  generatedAt: requireEnv('METADATA_GENERATED_AT'),
  commit: requireEnv('METADATA_COMMIT'),
  prerelease: /^true$/i.test(requireEnv('METADATA_PRERELEASE')),
  device: requireEnv('METADATA_DEVICE'),
  assets
}

const outputPath = path.resolve(requireEnv('METADATA_OUTPUT'))
fs.mkdirSync(path.dirname(outputPath), { recursive: true })
fs.writeFileSync(outputPath, `${JSON.stringify(metadata, null, 2)}\n`, 'utf8')