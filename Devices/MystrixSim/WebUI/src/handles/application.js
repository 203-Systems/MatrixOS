function getModule() {
  return window.Module ?? null
}

function readModuleString(exportName) {
  const mod = getModule()
  const reader = mod?.[exportName]
  if (!reader || !mod?.UTF8ToString) return ''

  const ptr = reader()
  if (!ptr) return ''

  return mod.UTF8ToString(ptr).trim()
}

export function getApplicationState() {
  const mod = getModule()
  if (!mod?._MatrixOS_Wasm_GetApplicationListJson || !mod?.UTF8ToString) {
    return { activeApp: null, activeAppId: null, applications: [], available: false }
  }

  const ptr = mod._MatrixOS_Wasm_GetApplicationListJson()
  if (!ptr) {
    return { activeApp: null, activeAppId: null, applications: [], available: true }
  }

  try {
    const parsed = JSON.parse(mod.UTF8ToString(ptr))
    return {
      activeApp: parsed.activeApp ?? null,
      activeAppId: parsed.activeAppId ?? null,
      applications: Array.isArray(parsed.applications) ? parsed.applications : [],
      available: true,
    }
  } catch {
    return { activeApp: null, activeAppId: null, applications: [], available: true }
  }
}

export function launchApplication(appId) {
  const mod = getModule()
  if (!mod?._MatrixOS_Wasm_LaunchApp) return false

  mod._MatrixOS_Wasm_LaunchApp(appId >>> 0)
  return true
}

export function getActiveAppSummary() {
  const name = readModuleString('_MatrixOS_Wasm_GetActiveAppName')
  if (!name) return null

  const author = readModuleString('_MatrixOS_Wasm_GetActiveAppAuthor')

  return {
    author,
    name,
    id: author ? `${author}-${name}` : name,
  }
}

export function isActiveApp(author, name) {
  const activeApp = getActiveAppSummary()
  if (!activeApp) return false
  if (activeApp.name !== name) return false
  return !author || activeApp.author === author
}
