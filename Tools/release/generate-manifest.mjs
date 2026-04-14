#!/usr/bin/env node

import fs from 'node:fs'
import path from 'node:path'

function parseArgs(argv) {
  const args = {}
  for (let index = 2; index < argv.length; index += 1) {
    const key = argv[index]
    if (!key.startsWith('--')) {
      throw new Error(`Unexpected argument: ${key}`)
    }

    const value = argv[index + 1]
    if (!value || value.startsWith('--')) {
      throw new Error(`Missing value for ${key}`)
    }

    args[key.slice(2)] = value
    index += 1
  }
  return args
}

function readJsonIfPresent(filePath) {
  if (!filePath || !fs.existsSync(filePath)) {
    return null
  }

  return JSON.parse(fs.readFileSync(filePath, 'utf8'))
}

function ensureArray(value) {
  return Array.isArray(value) ? value : []
}

function normalizeRelease(release) {
  return {
    tag: release.tag,
    name: release.name,
    channel: release.channel,
    channelSlug: release.channelSlug,
    version: release.version,
    displayVersion: release.displayVersion,
    date: release.date,
    generatedAt: release.generatedAt,
    commit: release.commit,
    prerelease: Boolean(release.prerelease),
    devices: release.devices && typeof release.devices === 'object' ? release.devices : {}
  }
}

function buildAssetUrl(repository, tag, assetName) {
  return `https://github.com/${repository}/releases/download/${tag}/${assetName}`
}

const args = parseArgs(process.argv)

if (!args['metadata-dir'] || !args.output || !args.repository) {
  throw new Error('Usage: generate-manifest.mjs --metadata-dir <dir> --output <file> --repository <owner/repo> [--generated-at <iso>] [--existing <file>]')
}

const metadataDir = path.resolve(args['metadata-dir'])
const outputPath = path.resolve(args.output)
const existingManifest = readJsonIfPresent(args.existing)
const generatedAt = args['generated-at'] || new Date().toISOString()

const releases = new Map()

for (const release of ensureArray(existingManifest?.releases)) {
  if (!release?.tag) {
    continue
  }
  releases.set(release.tag, normalizeRelease(release))
}

const metadataFiles = fs.existsSync(metadataDir)
  ? fs.readdirSync(metadataDir).filter((fileName) => fileName.endsWith('.json'))
  : []

for (const fileName of metadataFiles) {
  const metadata = JSON.parse(fs.readFileSync(path.join(metadataDir, fileName), 'utf8'))
  if (!metadata.tag || !metadata.device) {
    continue
  }

  const current = releases.get(metadata.tag) || normalizeRelease(metadata)
  current.tag = metadata.tag
  current.name = metadata.name || current.name
  current.channel = metadata.channel || current.channel
  current.channelSlug = metadata.channelSlug || current.channelSlug
  current.version = metadata.version || current.version
  current.displayVersion = metadata.displayVersion || current.displayVersion
  current.date = metadata.date || current.date
  current.generatedAt = metadata.generatedAt || current.generatedAt
  current.commit = metadata.commit || current.commit
  current.prerelease = Boolean(metadata.prerelease)

  const deviceAssets = {}
  for (const [assetKey, asset] of Object.entries(metadata.assets || {})) {
    if (!asset?.name) {
      continue
    }

    deviceAssets[assetKey] = {
      name: asset.name,
      sha256: asset.sha256 || '',
      url: asset.url || buildAssetUrl(args.repository, metadata.tag, asset.name)
    }
  }

  current.devices[metadata.device] = deviceAssets
  releases.set(metadata.tag, current)
}

const manifest = {
  schemaVersion: 1,
  generatedAt,
  releases: Array.from(releases.values()).sort((left, right) => {
    const leftTime = Date.parse(left.generatedAt || left.date || 0)
    const rightTime = Date.parse(right.generatedAt || right.date || 0)
    return rightTime - leftTime
  })
}

fs.mkdirSync(path.dirname(outputPath), { recursive: true })
fs.writeFileSync(outputPath, `${JSON.stringify(manifest, null, 2)}\n`, 'utf8')