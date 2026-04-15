const PREF_KEY = 'matrixos-firmware-source-v1'
const DB_NAME = 'matrixos-firmware-source-db'
const DB_VERSION = 1
const STORE_NAME = 'packages'

export const FIRMWARE_SOURCE = Object.freeze({
  LOCAL_BUILD: 'local-build',
  OFFICIAL_RELEASE: 'official-release',
  USER_SELECTED: 'user-selected',
})

const DEFAULT_PREFS = Object.freeze({
  selectedSource: FIRMWARE_SOURCE.LOCAL_BUILD,
  autoUseLocalBuild: true,
  localBuildHash: '',
  sources: {
    [FIRMWARE_SOURCE.OFFICIAL_RELEASE]: null,
    [FIRMWARE_SOURCE.USER_SELECTED]: null,
  },
})

let dbPromise = null

function clonePrefs(value) {
  return {
    selectedSource: value.selectedSource,
    autoUseLocalBuild: value.autoUseLocalBuild,
    localBuildHash: value.localBuildHash,
    sources: {
      [FIRMWARE_SOURCE.OFFICIAL_RELEASE]: value.sources?.[FIRMWARE_SOURCE.OFFICIAL_RELEASE] || null,
      [FIRMWARE_SOURCE.USER_SELECTED]: value.sources?.[FIRMWARE_SOURCE.USER_SELECTED] || null,
    },
  }
}

export function readFirmwareSourcePrefs() {
  try {
    const raw = window.localStorage.getItem(PREF_KEY)
    if (!raw) return clonePrefs(DEFAULT_PREFS)

    const parsed = JSON.parse(raw)
    return clonePrefs({
      ...DEFAULT_PREFS,
      ...parsed,
      sources: {
        ...DEFAULT_PREFS.sources,
        ...(parsed?.sources || {}),
      },
    })
  } catch {
    return clonePrefs(DEFAULT_PREFS)
  }
}

export function writeFirmwareSourcePrefs(value) {
  const next = clonePrefs(value)
  try {
    window.localStorage.setItem(PREF_KEY, JSON.stringify(next))
  } catch {}
  return next
}

function openDatabase() {
  if (!('indexedDB' in window)) {
    return Promise.reject(new Error('IndexedDB is not available in this browser.'))
  }

  if (dbPromise) return dbPromise

  dbPromise = new Promise((resolve, reject) => {
    const request = window.indexedDB.open(DB_NAME, DB_VERSION)

    request.onupgradeneeded = () => {
      const db = request.result
      if (!db.objectStoreNames.contains(STORE_NAME)) {
        db.createObjectStore(STORE_NAME, { keyPath: 'slot' })
      }
    }

    request.onsuccess = () => resolve(request.result)
    request.onerror = () => reject(request.error || new Error('Failed to open firmware package database.'))
  })

  return dbPromise
}

export async function saveStoredPackage(slot, record) {
  const db = await openDatabase()
  await new Promise((resolve, reject) => {
    const tx = db.transaction(STORE_NAME, 'readwrite')
    tx.objectStore(STORE_NAME).put({ slot, ...record })
    tx.oncomplete = () => resolve()
    tx.onerror = () => reject(tx.error || new Error('Failed to persist firmware package.'))
  })
}

export async function loadStoredPackage(slot) {
  const db = await openDatabase()
  return await new Promise((resolve, reject) => {
    const tx = db.transaction(STORE_NAME, 'readonly')
    const request = tx.objectStore(STORE_NAME).get(slot)
    request.onsuccess = () => resolve(request.result || null)
    request.onerror = () => reject(request.error || new Error('Failed to load stored firmware package.'))
  })
}
