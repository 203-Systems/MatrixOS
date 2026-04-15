import { get, writable } from 'svelte/store'
import { buildMetadata, loadRuntimeAssetPair, moduleReady } from './wasm.js'
import {
  FIRMWARE_SOURCE,
  loadStoredPackage,
  readFirmwareSourcePrefs,
  writeFirmwareSourcePrefs,
} from './firmwareSource.js'
import { extractFirmwarePackage, FIRMWARE_PACKAGE_SUFFIX, sha256Hex } from '../firmwarePackage.js'

const LOCAL_BUILD_PACKAGE_URL = `/MatrixOS${FIRMWARE_PACKAGE_SUFFIX}`

export const activeFirmwareSource = writable(/** @type {string} */ (FIRMWARE_SOURCE.LOCAL_BUILD))

let initializationPromise = null

function currentRuntimeLoaded() {
  const currentFirmwareMeta = get(buildMetadata)
  return get(moduleReady)
    || currentFirmwareMeta.version !== '—'
    || currentFirmwareMeta.buildType !== '—'
    || currentFirmwareMeta.buildHash !== '—'
}

async function fetchPackageBytes(url, init = {}) {
  const response = await fetch(url, init)
  if (!response.ok) {
    throw new Error(`Package request failed: HTTP ${response.status}`)
  }
  return await response.arrayBuffer()
}

async function readLocalBuildPackage() {
  try {
    const bytes = await fetchPackageBytes(LOCAL_BUILD_PACKAGE_URL, { cache: 'no-store' })
    const packageHash = await sha256Hex(bytes)
    return {
      bytes,
      metadata: {
        label: `MatrixOS${FIRMWARE_PACKAGE_SUFFIX}`,
        packageHash,
        savedAt: new Date().toISOString(),
      },
    }
  } catch {
    return null
  }
}

async function activatePackageSource(sourceId, { bytes, label, metadata }) {
  const runtimeAssets = await extractFirmwarePackage(bytes, label)
  await loadRuntimeAssetPair({ ...runtimeAssets, label })
  activeFirmwareSource.set(sourceId)

  return {
    ...(metadata || {}),
    label: metadata?.label || label,
    packageHash: metadata?.packageHash || await sha256Hex(bytes),
    savedAt: metadata?.savedAt || new Date().toISOString(),
  }
}

async function restoreStoredSource(sourceId) {
  try {
    const stored = await loadStoredPackage(sourceId)
    if (!stored?.packageBytes) {
      return false
    }

    const label = stored.label || stored.metadata?.label || `MatrixOS${FIRMWARE_PACKAGE_SUFFIX}`
    await activatePackageSource(sourceId, {
      bytes: stored.packageBytes,
      label,
      metadata: stored.metadata || null,
    })
    return true
  } catch {
    return false
  }
}

function persistPrefsWith(nextValues) {
  const currentPrefs = readFirmwareSourcePrefs()
  return writeFirmwareSourcePrefs({
    ...currentPrefs,
    ...nextValues,
    sources: {
      ...currentPrefs.sources,
      ...(nextValues.sources || {}),
    },
  })
}

async function initializeFirmwareRuntime() {
  const prefs = readFirmwareSourcePrefs()
  const selectedSource = prefs.selectedSource || FIRMWARE_SOURCE.LOCAL_BUILD
  activeFirmwareSource.set(selectedSource)

  const localBuildPackage = await readLocalBuildPackage()
  const nextLocalHash = localBuildPackage?.metadata?.packageHash || ''
  const localBuildChanged = Boolean(nextLocalHash) && nextLocalHash !== prefs.localBuildHash

  if (nextLocalHash && nextLocalHash !== prefs.localBuildHash) {
    persistPrefsWith({ localBuildHash: nextLocalHash })
  }

  if (prefs.autoUseLocalBuild && localBuildPackage && localBuildChanged) {
    const nextMetadata = await activatePackageSource(FIRMWARE_SOURCE.LOCAL_BUILD, {
      bytes: localBuildPackage.bytes,
      label: `MatrixOS${FIRMWARE_PACKAGE_SUFFIX}`,
      metadata: localBuildPackage.metadata,
    })
    persistPrefsWith({
      selectedSource: FIRMWARE_SOURCE.LOCAL_BUILD,
      localBuildHash: nextMetadata.packageHash,
    })
    return
  }

  if (selectedSource === FIRMWARE_SOURCE.LOCAL_BUILD) {
    activeFirmwareSource.set(FIRMWARE_SOURCE.LOCAL_BUILD)
    if (!currentRuntimeLoaded() && localBuildPackage) {
      const nextMetadata = await activatePackageSource(FIRMWARE_SOURCE.LOCAL_BUILD, {
        bytes: localBuildPackage.bytes,
        label: `MatrixOS${FIRMWARE_PACKAGE_SUFFIX}`,
        metadata: localBuildPackage.metadata,
      })
      persistPrefsWith({ localBuildHash: nextMetadata.packageHash })
    }
    return
  }

  if (selectedSource === FIRMWARE_SOURCE.OFFICIAL_RELEASE) {
    if (currentRuntimeLoaded() && get(activeFirmwareSource) === FIRMWARE_SOURCE.OFFICIAL_RELEASE) {
      return
    }
    await restoreStoredSource(FIRMWARE_SOURCE.OFFICIAL_RELEASE)
    return
  }

  if (selectedSource === FIRMWARE_SOURCE.USER_SELECTED) {
    if (currentRuntimeLoaded() && get(activeFirmwareSource) === FIRMWARE_SOURCE.USER_SELECTED) {
      return
    }
    await restoreStoredSource(FIRMWARE_SOURCE.USER_SELECTED)
  }
}

export function setActiveFirmwareSource(sourceId) {
  activeFirmwareSource.set(sourceId)
}

export async function ensureFirmwareRuntimeSource() {
  if (initializationPromise) {
    return initializationPromise
  }

  initializationPromise = initializeFirmwareRuntime().finally(() => {
    initializationPromise = null
  })

  return initializationPromise
}