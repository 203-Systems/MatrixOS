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
