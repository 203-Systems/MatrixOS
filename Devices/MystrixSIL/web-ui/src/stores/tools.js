// Tool selection state for the right-side tool tray
import { writable } from 'svelte/store'

const STORAGE_KEY = 'matrixos-open-tools'

function loadOpenTools() {
  try {
    const raw = window.localStorage.getItem(STORAGE_KEY)
    if (raw) {
      const parsed = JSON.parse(raw)
      if (Array.isArray(parsed)) return parsed
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

// Tool registry for the Device page
export const deviceTools = [
  { id: 'application', label: 'Application' },
  { id: 'input',       label: 'Input' },
  { id: 'logs',        label: 'Logs' },
  { id: 'runtime',     label: 'Runtime' },
  { id: 'ui',          label: 'UI' },
  { id: 'midi',        label: 'MIDI' },
  { id: 'hid',         label: 'HID' },
  { id: 'serial',      label: 'Serial' },
  { id: 'usage',       label: 'Usage' },
  { id: 'usb',         label: 'USB' },
  { id: 'gyro',        label: 'Gyro' },
  { id: 'battery',     label: 'Battery' },
  { id: 'storage',     label: 'Storage' },
]
