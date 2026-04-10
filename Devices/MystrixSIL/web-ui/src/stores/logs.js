// Structured log store for MystrixSIL dashboard
import { writable, derived } from 'svelte/store'

const ansiRegex = /\x1B\[[0-?]*[ -/]*[@-~]/g
const ansiColorMap = {
  30: '#60656f', 31: '#ff6b6b', 32: '#6bdd8b', 33: '#f7c266',
  34: '#6ba7ff', 35: '#d47fff', 36: '#5ad4ff', 37: '#e6e6ea',
  90: '#9ea1ad', 91: '#ff8b8b', 92: '#7ff7a1', 93: '#ffd27f',
  94: '#86b8ff', 95: '#e2a0ff', 96: '#8de5ff', 97: '#ffffff'
}

let counter = 0
const MAX_LOGS = 500

export const logMessages = writable([])
export const logFilter = writable('')
export const logLevelFilter = writable('all')
export const errorCount = writable(0)
export const warnCount = writable(0)

export const filteredLogs = derived(
  [logMessages, logFilter, logLevelFilter],
  ([$msgs, $filter, $level]) => {
    let result = $msgs
    if ($level !== 'all') result = result.filter(m => m.level === $level)
    if ($filter) {
      const lower = $filter.toLowerCase()
      result = result.filter(m => m.text.toLowerCase().includes(lower))
    }
    return result
  }
)

function normalizeLog(raw) {
  const matches = [...raw.matchAll(/\x1B\[([0-9;]+)m/g)]
  let color = null
  if (matches.length > 0) {
    const last = matches[matches.length - 1][1]
    const parts = last.split(';').map(v => parseInt(v, 10)).filter(v => !Number.isNaN(v))
    const code = parts.find(v => v !== 0)
    if (code && ansiColorMap[code]) color = ansiColorMap[code]
  }
  const text = raw.replace(ansiRegex, '').trim()
  return { text, color }
}

function isMatrixOSLog(text) {
  if (!text) return false
  if (text.startsWith('[MystrixSIL]')) return true
  return /^[DIWEV]\s*\(\d+\)/.test(text)
}

function timestamp() {
  const now = new Date()
  return now.toLocaleTimeString('en-US', {
    hour12: false, hour: '2-digit', minute: '2-digit', second: '2-digit'
  }) + '.' + String(now.getMilliseconds()).padStart(3, '0')
}

export function pushLog(level, args, source = 'console') {
  const toStr = (a) => {
    if (a === null || a === undefined) return String(a)
    if (typeof a === 'object') { try { return JSON.stringify(a) } catch { return '[object]' } }
    return String(a)
  }

  const raw = args.map(toStr).join(' ')
  const { text, color: ansiColor } = normalizeLog(raw)

  if (source !== 'matrixos' && !isMatrixOSLog(text)) return

  const suppressed =
    text.includes('worker sent an error!') ||
    text.includes('RuntimeError: table index is out of bounds')
  if (suppressed || !text) return

  const levelColor = {
    error: ansiColorMap[91], warn: ansiColorMap[93],
    info: ansiColorMap[96], log: null
  }[level]

  let prefixColor = null
  const prefixMatch = text.match(/^([DIWE])\s*[(:]/)
  if (prefixMatch) {
    const tag = prefixMatch[1]
    if (tag === 'E') prefixColor = ansiColorMap[91]
    else if (tag === 'W') prefixColor = ansiColorMap[93]
    else if (tag === 'I') prefixColor = ansiColorMap[92]
    else if (tag === 'D') prefixColor = ansiColorMap[96]
  }

  if (level === 'error') errorCount.update(n => n + 1)
  if (level === 'warn') warnCount.update(n => n + 1)

  const entry = {
    id: counter++,
    level,
    text,
    timestamp: timestamp(),
    color: ansiColor || prefixColor || levelColor
  }

  logMessages.update(msgs => {
    const trimmed = msgs.length >= MAX_LOGS ? msgs.slice(-MAX_LOGS + 1) : msgs
    return [...trimmed, entry]
  })
}

export function clearLogs() {
  logMessages.set([])
  errorCount.set(0)
  warnCount.set(0)
}

export function hookModuleLogging() {
  window.MatrixOSLogDispatch = (level, args) => {
    pushLog(level, args, 'matrixos')
  }
  if (window.MatrixOSLogBuffer && Array.isArray(window.MatrixOSLogBuffer)) {
    window.MatrixOSLogBuffer.forEach(entry => {
      if (entry?.level && entry?.args) pushLog(entry.level, entry.args, 'matrixos')
    })
    window.MatrixOSLogBuffer.length = 0
  }
  return () => { window.MatrixOSLogDispatch = null }
}
