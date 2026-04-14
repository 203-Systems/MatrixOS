const BUILD_IDENTITY_SEPARATOR = '•'

export const EMPTY_BUILD_METADATA = Object.freeze({
  version: '—',
  buildType: '—',
  buildHash: '—',
  buildHashRaw: '',
  buildTime: '—',
  buildIdentity: 'Matrix OS',
  channel: '',
  releaseVersion: 0,
  dirty: false,
})

function trimText(value) {
  return typeof value === 'string' ? value.trim() : ''
}

function deriveChannel(buildType) {
  const value = trimText(buildType).toLowerCase()
  if (value.includes('release candidate') || /^rc(?:\s|$)/.test(value)) return 'release-candidate'
  if (value.includes('beta')) return 'beta'
  if (value.includes('nightly')) return 'nightly'
  if (value.includes('indev') || value.includes('dev')) return 'indev'
  if (value.includes('release')) return 'release'
  return ''
}

function formatBuildHash(buildHashRaw, dirty) {
  const hash = trimText(buildHashRaw)
  if (hash) return dirty ? `${hash} (Dirty)` : hash
  return dirty ? '(Dirty)' : EMPTY_BUILD_METADATA.buildHash
}

function composeBuildIdentity(version, buildType, buildHashRaw, dirty) {
  let identity = trimText(version) || EMPTY_BUILD_METADATA.buildIdentity
  if (buildType && buildType !== EMPTY_BUILD_METADATA.buildType) {
    identity += ` ${BUILD_IDENTITY_SEPARATOR} ${buildType}`
  }
  if (buildHashRaw) {
    identity += ` ${buildHashRaw}`
  }
  if (dirty) {
    identity += ` ${BUILD_IDENTITY_SEPARATOR} Dirty`
  }
  return identity
}

export function normalizeBuildMetadata(raw = {}) {
  const version = trimText(raw.version) || EMPTY_BUILD_METADATA.version
  const buildType = trimText(raw.buildType) || EMPTY_BUILD_METADATA.buildType
  const buildHashRaw = trimText(raw.buildHashRaw || raw.gitHash)
  const buildTime = trimText(raw.buildTime) || EMPTY_BUILD_METADATA.buildTime
  const dirty = Boolean(raw.dirty)
  const releaseVersionValue = Number(raw.releaseVersion)
  const releaseVersion = Number.isFinite(releaseVersionValue) ? releaseVersionValue : EMPTY_BUILD_METADATA.releaseVersion
  const channel = trimText(raw.channel) || deriveChannel(buildType)
  const buildIdentity = trimText(raw.buildIdentity) || composeBuildIdentity(
    version === EMPTY_BUILD_METADATA.version ? EMPTY_BUILD_METADATA.buildIdentity : version,
    buildType,
    buildHashRaw,
    dirty,
  )

  return {
    version,
    buildType,
    buildHash: formatBuildHash(buildHashRaw, dirty),
    buildHashRaw,
    buildTime,
    buildIdentity,
    channel,
    releaseVersion,
    dirty,
  }
}

export function parseBuildMetadataJson(jsonText) {
  try {
    return normalizeBuildMetadata(JSON.parse(jsonText))
  } catch {
    return null
  }
}

export function parseBuildIdentity(buildIdentity, { buildTime = EMPTY_BUILD_METADATA.buildTime, version = EMPTY_BUILD_METADATA.version } = {}) {
  const identity = trimText(buildIdentity)
  const fallbackVersion = trimText(version)

  if (!identity) {
    return normalizeBuildMetadata({
      version: fallbackVersion,
      buildTime,
      buildIdentity: fallbackVersion || EMPTY_BUILD_METADATA.buildIdentity,
    })
  }

  const parts = identity.split(BUILD_IDENTITY_SEPARATOR).map((part) => part.trim()).filter(Boolean)
  if (parts.length === 0) {
    return normalizeBuildMetadata({
      version: fallbackVersion,
      buildTime,
      buildIdentity: identity,
    })
  }

  const dirty = parts.some((part) => /^dirty$/i.test(part))
  const detail = parts.slice(1).filter((part) => !/^dirty$/i.test(part)).join(' ').trim()

  let buildType = ''
  let buildHashRaw = ''
  if (detail) {
    const hashMatch = detail.match(/(?:^|\s)([0-9a-f]{7,40})$/i)
    if (hashMatch) {
      buildHashRaw = hashMatch[1]
      buildType = detail.slice(0, hashMatch.index).trim()
    } else {
      buildType = detail
    }
  }

  return normalizeBuildMetadata({
    version: parts[0] || fallbackVersion,
    buildType,
    buildHashRaw,
    buildTime,
    buildIdentity: identity,
    dirty,
  })
}

export function buildTypeClass(buildType) {
  const value = trimText(buildType).toLowerCase()
  if (value.includes('release candidate') || /^rc(?:\s|$)/.test(value)) return 'build-type-rc'
  if (value.includes('beta')) return 'build-type-beta'
  if (value.includes('nightly')) return 'build-type-nightly'
  if (value.includes('indev') || value.includes('dev')) return 'build-type-indev'
  if (value.includes('release')) return 'build-type-release'
  return 'build-type-neutral'
}