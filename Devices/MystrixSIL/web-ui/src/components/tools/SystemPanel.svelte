<script>
  import { onMount } from 'svelte'
  import { formatBytes, usageSnapshot } from '../../stores/tooling.js'
  import { moduleReady, wasmMissing, runtimeStatus, getUptimeMs, buildIdentity } from '../../stores/wasm.js'
  import { errorCount, warnCount, logMessages } from '../../stores/logs.js'
  export let showHero = true
  export let onCloseHero = () => {}

  let uptime = '—'
  let uptimeTimer

  function parseBuildIdentity(identity) {
    const fallback = {
      version: identity || '—',
      buildType: '—',
      buildHash: '—'
    }

    if (!identity) return fallback

    const parts = identity.split('•').map((part) => part.trim()).filter(Boolean)
    if (!parts.length) return fallback

    const version = parts[0] || fallback.version
    const dirty = parts.some((part) => /^dirty$/i.test(part))
    const remainder = parts.slice(1).join(' ').trim()

    if (!remainder) {
      return {
        version,
        buildType: '—',
        buildHash: dirty ? '(Dirty)' : '—'
      }
    }

    const hashMatch = remainder.match(/\b([0-9a-f]{7,40})\b/i)
    const buildHash = hashMatch ? `${hashMatch[1]}${dirty ? ' (Dirty)' : ''}` : (dirty ? `${remainder} (Dirty)` : remainder)
    const buildType = hashMatch ? remainder.slice(0, hashMatch.index).trim() || '—' : remainder

    return {
      version,
      buildType,
      buildHash
    }
  }

  $: buildMeta = parseBuildIdentity($buildIdentity)

  function formatUptime(ms) {
    if (!ms) return '—'
    const s = Math.floor(ms / 1000)
    const m = Math.floor(s / 60)
    const h = Math.floor(m / 60)
    if (h > 0) return `${h}h ${m % 60}m ${s % 60}s`
    if (m > 0) return `${m}m ${s % 60}s`
    return `${s}s`
  }

  onMount(() => {
    uptime = formatUptime(getUptimeMs())
    uptimeTimer = setInterval(() => { uptime = formatUptime(getUptimeMs()) }, 1000)
    return () => clearInterval(uptimeTimer)
  })
</script>

<div class="tool-surface">
  {#if showHero}
  <section class="tool-hero">
    <button class="tool-hero-close" on:click={onCloseHero} title="Close">✕</button>
    <div class="tool-hero-title">System</div>
    <div class="tool-hero-desc">
      Live runtime status, health counters, and planned telemetry hooks for the MystrixSIL WASM environment.
    </div>
  </section>
  {/if}

  <section class="tool-section">
    <div class="tool-section-title">Runtime</div>
    <div class="tool-grid">
      <div class="tool-card">
        <span class="tool-card-label">Runtime</span>
        <span class="tool-card-value" class:tool-value-live={$moduleReady} class:tool-value-warn={!$moduleReady}>
          {$runtimeStatus}
        </span>
      </div>
      <div class="tool-card">
        <span class="tool-card-label">OS Version</span>
        <span class="tool-card-value" class:tool-value-live={!$wasmMissing} class:tool-value-error={$wasmMissing}>
          {$wasmMissing ? 'Missing' : buildMeta.version}
        </span>
      </div>
      <div class="tool-card">
        <span class="tool-card-label">Build Type</span>
        <span class="tool-card-value" class:tool-value-live={!$wasmMissing} class:tool-value-error={$wasmMissing}>
          {$wasmMissing ? 'Missing' : buildMeta.buildType}
        </span>
      </div>
      <div class="tool-card">
        <span class="tool-card-label">Build Hash</span>
        <span class="tool-card-value" class:tool-value-live={!$wasmMissing} class:tool-value-error={$wasmMissing}>
          {$wasmMissing ? 'Missing' : buildMeta.buildHash}
        </span>
      </div>
    </div>
  </section>

  <section class="tool-section">
    <div class="tool-section-title">Health</div>
    <div class="tool-grid">
      <div class="tool-card">
        <span class="tool-card-label">WASM heap</span>
        <span class="tool-card-value">{formatBytes($usageSnapshot.heapBytes)}</span>
      </div>
      <div class="tool-card">
        <span class="tool-card-label">Errors</span>
        <span class="tool-card-value" class:tool-value-error={$errorCount > 0}>{$errorCount}</span>
      </div>
      <div class="tool-card">
        <span class="tool-card-label">Warnings</span>
        <span class="tool-card-value" class:tool-value-warn={$warnCount > 0}>{$warnCount}</span>
      </div>
      <div class="tool-card">
        <span class="tool-card-label">Log entries</span>
        <span class="tool-card-value">{$logMessages.length}</span>
      </div>
      <div class="tool-card">
        <span class="tool-card-label">Uptime</span>
        <span class="tool-card-value">{uptime}</span>
      </div>
    </div>
  </section>

</div>

<style>
</style>
