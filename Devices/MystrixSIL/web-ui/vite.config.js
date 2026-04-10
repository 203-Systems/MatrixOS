import { defineConfig } from 'vite'
import { svelte } from '@sveltejs/vite-plugin-svelte'
import { execSync } from 'child_process'

function gitInfo() {
  try {
    const hash = execSync('git rev-parse --short HEAD', { encoding: 'utf-8' }).trim()
    let dirty = false
    try {
      execSync('git diff --quiet HEAD', { encoding: 'utf-8' })
    } catch {
      dirty = true
    }
    return { hash, dirty }
  } catch {
    return { hash: '', dirty: false }
  }
}

const git = gitInfo()

// https://vite.dev/config/
export default defineConfig({
  plugins: [svelte()],
  define: {
    __GIT_HASH__: JSON.stringify(git.hash),
    __GIT_DIRTY__: git.dirty,
  },
  server: {
    headers: {
      'Cross-Origin-Opener-Policy': 'same-origin',
      'Cross-Origin-Embedder-Policy': 'require-corp'
    }
  },
  preview: {
    headers: {
      'Cross-Origin-Opener-Policy': 'same-origin',
      'Cross-Origin-Embedder-Policy': 'require-corp'
    }
  }
})
