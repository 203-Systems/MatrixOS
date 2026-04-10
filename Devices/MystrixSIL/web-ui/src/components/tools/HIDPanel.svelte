<script>
  import { activeGridKeys, fnKeyActive, runtimeGridKeys, runtimeFnActive } from '../../stores/input.js'
  import { browserCapabilities, hidHooks, usageSnapshot } from '../../stores/tooling.js'

  $: injectedKeys = [
    ...($fnKeyActive ? ['FN'] : []),
    ...[...$activeGridKeys].map(key => `(${key})`)
  ]

  $: runtimeKeys = [
    ...($runtimeFnActive ? ['FN'] : []),
    ...[...$runtimeGridKeys].map(key => `(${key})`)
  ]
</script>

<div class="tool-surface">
  <section class="tool-hero">
    <div class="tool-hero-title">HID Inspector</div>
    <div class="tool-hero-desc">
      HID now reflects the live local input path and the browser capabilities around it, so future WebHID and gamepad adapters have a clear landing zone.
    </div>
  </section>

  <section class="tool-section">
    <div class="tool-section-title">Current posture</div>
    <div class="tool-grid">
      <div class="tool-card">
        <span class="tool-card-label">Local injection</span>
        <span class="tool-card-value tool-value-live">Live</span>
      </div>
      <div class="tool-card">
        <span class="tool-card-label">WebHID</span>
        <span class="tool-card-value" class:tool-value-live={$browserCapabilities.hid} class:tool-value-warn={!$browserCapabilities.hid}>
          {$browserCapabilities.detected ? ($browserCapabilities.hid ? 'Available' : 'Unavailable') : 'Detecting'}
        </span>
      </div>
      <div class="tool-card">
        <span class="tool-card-label">Gamepad API</span>
        <span class="tool-card-value" class:tool-value-live={$browserCapabilities.gamepad} class:tool-value-warn={!$browserCapabilities.gamepad}>
          {$browserCapabilities.detected ? ($browserCapabilities.gamepad ? 'Available' : 'Unavailable') : 'Detecting'}
        </span>
      </div>
      <div class="tool-card">
        <span class="tool-card-label">Runtime-held inputs</span>
        <span class="tool-card-value">{$usageSnapshot.runtimeHeldCount}</span>
      </div>
    </div>
  </section>

  <section class="tool-section">
    <div class="tool-section-title">Current activity</div>
    <div class="tool-grid">
      <div class="tool-card">
        <span class="tool-card-label">Injected</span>
        {#if injectedKeys.length > 0}
          <div class="tool-tag-row">
            {#each injectedKeys as key}
              <span class="tool-tag">{key}</span>
            {/each}
          </div>
        {:else}
          <span class="tool-empty">No injected inputs are currently held.</span>
        {/if}
      </div>
      <div class="tool-card">
        <span class="tool-card-label">Runtime-visible</span>
        {#if runtimeKeys.length > 0}
          <div class="tool-tag-row">
            {#each runtimeKeys as key}
              <span class="tool-tag">{key}</span>
            {/each}
          </div>
        {:else}
          <span class="tool-empty">The OS does not currently see any held HID-style inputs.</span>
        {/if}
      </div>
    </div>
  </section>

  <section class="tool-section">
    <div class="tool-section-title">Adapter hooks</div>
    <div class="tool-list">
      {#each hidHooks as hook}
        <div class="tool-list-item">
          <div class="tool-list-main">
            <span class="tool-list-title">{hook.title}</span>
            <span class="tool-list-detail">{hook.detail}</span>
          </div>
          <span class="status-pill status-{hook.status}">{hook.label}</span>
        </div>
      {/each}
    </div>
  </section>
</div>
