<script>
  import { browserCapabilities, midiHooks } from '../../stores/tooling.js'

  $: midiSupport = !$browserCapabilities.detected
    ? { label: 'Detecting', tone: 'idle' }
    : $browserCapabilities.midi
      ? { label: 'Available', tone: 'live' }
      : { label: 'Unavailable', tone: 'blocked' }

  $: secureContext = !$browserCapabilities.detected
    ? { label: 'Detecting', tone: 'idle' }
    : $browserCapabilities.secureContext
      ? { label: 'Secure', tone: 'live' }
      : { label: 'Insecure', tone: 'blocked' }
</script>

<div class="tool-surface">
  <section class="tool-hero">
    <div class="tool-hero-title">MIDI Monitor</div>
    <div class="tool-hero-desc">
      The panel now documents the real browser/runtime readiness for MIDI work and preserves a clean protocol-aware lane for monitoring and future injection.
    </div>
    <div class="tool-tag-row">
      <span class="tool-tag">note on/off</span>
      <span class="tool-tag">CC</span>
      <span class="tool-tag">clock</span>
      <span class="tool-tag">sysex gated</span>
    </div>
  </section>

  <section class="tool-section">
    <div class="tool-section-title">Runtime posture</div>
    <div class="tool-grid">
      <div class="tool-card">
        <span class="tool-card-label">Web MIDI</span>
        <span class="tool-card-value tool-value-{midiSupport.tone}">{midiSupport.label}</span>
      </div>
      <div class="tool-card">
        <span class="tool-card-label">Context</span>
        <span class="tool-card-value tool-value-{secureContext.tone}">{secureContext.label}</span>
      </div>
      <div class="tool-card">
        <span class="tool-card-label">Runtime bridge</span>
        <span class="tool-card-value tool-value-warn">Not wired yet</span>
      </div>
      <div class="tool-card">
        <span class="tool-card-label">Injection lane</span>
        <span class="tool-card-value">Panel contract ready</span>
      </div>
    </div>
  </section>

  <section class="tool-section">
    <div class="tool-section-title">Protocol lanes</div>
    <div class="tool-list">
      {#each midiHooks as hook}
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
