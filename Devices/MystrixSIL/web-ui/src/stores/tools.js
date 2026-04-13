// Tool selection state for the right-side tool tray
import { writable } from 'svelte/store'

// Tool registry for the Device page
export const deviceTools = [
  { id: 'system',    label: 'System' },
  { id: 'input',     label: 'Input' },
  { id: 'logs',      label: 'Logs' },
  { id: 'midi',      label: 'MIDI' },
  { id: 'hid',       label: 'HID' },
  { id: 'serial',    label: 'Serial' },
  { id: 'storage',   label: 'Storage' },
  { id: 'device-hw', label: 'Hardware' },
]

const STORAGE_KEY = 'matrixos-open-tools'
const validToolIds = new Set(deviceTools.map((tool) => tool.id))

function sanitizeOpenTools(value) {
  if (!Array.isArray(value)) return []

  const seen = new Set()
  return value.filter((id) => {
    if (!validToolIds.has(id) || seen.has(id)) return false
    seen.add(id)
    return true
  })
}

function loadOpenTools() {
  try {
    const raw = window.localStorage.getItem(STORAGE_KEY)
    if (raw) {
      const parsed = JSON.parse(raw)
      return sanitizeOpenTools(parsed)
    }
  } catch {}
  return []
}

export const openTools = writable(loadOpenTools())

openTools.subscribe(value => {
  try { window.localStorage.setItem(STORAGE_KEY, JSON.stringify(value)) } catch {}
})

export function toggleTool(id) {
  openTools.update(tools => {
    if (tools.includes(id)) {
      return tools.filter(t => t !== id)
    }
    return [...tools, id]
  })
}

export function closeTool(id) {
  openTools.update(tools => tools.filter(t => t !== id))
}
