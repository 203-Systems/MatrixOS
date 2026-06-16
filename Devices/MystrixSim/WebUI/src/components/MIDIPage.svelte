<script>
  import { onMount } from 'svelte'
  import { Add, Close, Music, Renew, StopFilledAlt } from 'carbon-icons-svelte'
  import MIDIPanel from './tools/MIDIPanel.svelte'
  import {
    addSynthBinding,
    auditionSynthBinding,
    auditionSynthPreset,
    midiForwarding,
    panicSynth,
    requestWebMidiAccess,
    resumeSynthAudio,
    synthBindings,
    synthCategories,
    synthPresets,
    updateMidiForwarding,
    updateSynthBinding,
    removeSynthBinding,
    webMidiInputs,
    webMidiOutputs,
    webMidiState,
  } from '../stores/midi.js'

  let voiceSelector = null
  let voiceDraft = ''
  let voiceTab = 'Piano'

  onMount(() => {
    if ($webMidiState.supported && !$webMidiState.requested) {
      void requestWebMidiAccess()
    }
  })

  $: inputStatus = (() => {
    if (!$webMidiState.supported) return 'Unavailable'
    if ($webMidiState.error) return 'Error'
    if (!$webMidiState.ready) return $webMidiState.requested ? 'Waiting' : 'Idle'
    return $midiForwarding.inputEnabled ? 'Forwarding' : 'Ready'
  })()

  $: outputStatus = (() => {
    if (!$webMidiState.supported) return 'Unavailable'
    if ($webMidiState.error) return 'Error'
    if (!$webMidiState.ready) return $webMidiState.requested ? 'Waiting' : 'Idle'
    return $midiForwarding.outputEnabled ? 'Forwarding' : 'Ready'
  })()

  function setForwarding(key, value) {
    updateMidiForwarding({ [key]: value })
  }

  function setBinding(id, key, value) {
    const next = key === 'channel' ? Math.max(1, Math.min(16, parseInt(value, 10) || 1)) : value
    updateSynthBinding(id, { [key]: next })
    if (key === 'enabled' && value) resumeSynthAudio()
  }

  function requestAccess() {
    void requestWebMidiAccess()
  }

  function addSynth() {
    addSynthBinding()
  }

  function panic() {
    panicSynth()
  }

  function currentPresetName(id) {
    return synthPresets.find(preset => preset.id === id)?.name || synthPresets[0]?.name || 'Instrument'
  }

  function openVoiceSelector(binding) {
    const preset = synthPresets.find(item => item.id === binding.preset) || synthPresets[0]
    voiceSelector = binding
    voiceDraft = preset.id
    voiceTab = preset.category
  }

  function closeVoiceSelector() {
    voiceSelector = null
    voiceDraft = ''
  }

  function auditionVoice(preset) {
    voiceDraft = preset.id
    auditionSynthPreset(preset.id, voiceSelector?.channel || 1)
  }

  function confirmVoice() {
    if (!voiceSelector || !voiceDraft) return
    setBinding(voiceSelector.id, 'preset', voiceDraft)
    closeVoiceSelector()
  }

  function handleBackdropClick(event) {
    if (event.target === event.currentTarget) closeVoiceSelector()
  }

  $: visiblePresets = synthPresets.filter(preset => preset.category === voiceTab)
</script>

<div class="tool-surface midi-page">
  <section class="tool-hero midi-hero">
    <div class="tool-hero-title-row">
      <Music size={18} />
      <div class="tool-hero-title">MIDI</div>
    </div>
    <div class="tool-hero-desc">Forward browser MIDI devices into Matrix Sim, mirror Matrix MIDI output to hardware ports, and bind output channels to lightweight synth voices.</div>
  </section>

  <section class="tool-section">
    <div class="tool-section-title-row">
      <div class="tool-section-title">Web MIDI Bridge</div>
      <button class="midi-icon-button" on:click={requestAccess} title="Refresh MIDI access">
        <Renew size={16} />
      </button>
    </div>

    <div class="tool-grid midi-status-grid">
      <div class="tool-card">
        <span class="tool-card-label">Browser MIDI</span>
        <span
          class="tool-card-value"
          class:tool-value-live={$webMidiState.ready}
          class:tool-value-error={$webMidiState.error}
          class:tool-value-idle={!$webMidiState.ready && !$webMidiState.error}
        >
          {$webMidiState.ready ? 'Ready' : ($webMidiState.supported ? 'Permission Needed' : 'Unsupported')}
        </span>
      </div>
      <div class="tool-card">
        <span class="tool-card-label">Matrix Input</span>
        <span class="tool-card-value" class:tool-value-live={$midiForwarding.inputEnabled}>{inputStatus}</span>
      </div>
      <div class="tool-card">
        <span class="tool-card-label">Matrix Output</span>
        <span class="tool-card-value" class:tool-value-live={$midiForwarding.outputEnabled}>{outputStatus}</span>
      </div>
      <div class="tool-card">
        <span class="tool-card-label">Synth Bindings</span>
        <span class="tool-card-value">{$synthBindings.filter(binding => binding.enabled).length} / {$synthBindings.length}</span>
      </div>
    </div>

    {#if $webMidiState.error}
      <div class="midi-error">{$webMidiState.error}</div>
    {/if}

    <div class="midi-bridge-grid">
      <div class="midi-route-card">
        <div class="midi-route-header">
          <div>
            <div class="midi-route-title">MIDI Port to Simulator Input</div>
            <div class="midi-route-subtitle">Browser MIDI input → simulator USB MIDI input</div>
          </div>
          <label class="midi-switch">
            <input
              type="checkbox"
              checked={$midiForwarding.inputEnabled}
              on:change={(event) => setForwarding('inputEnabled', event.currentTarget.checked)}
            />
            <span></span>
          </label>
        </div>
        <select
          class="midi-select"
          value={$midiForwarding.selectedInputId}
          disabled={!$webMidiState.ready || $webMidiInputs.length === 0}
          on:change={(event) => setForwarding('selectedInputId', event.currentTarget.value)}
        >
          <option value="none">None</option>
          {#each $webMidiInputs as input}
            <option value={input.id}>{input.name}</option>
          {/each}
        </select>
      </div>

      <div class="midi-route-card">
        <div class="midi-route-header">
          <div>
            <div class="midi-route-title">Simulator Output to MIDI Port</div>
            <div class="midi-route-subtitle">Simulator USB MIDI output → browser MIDI output</div>
          </div>
          <label class="midi-switch">
            <input
              type="checkbox"
              checked={$midiForwarding.outputEnabled}
              on:change={(event) => setForwarding('outputEnabled', event.currentTarget.checked)}
            />
            <span></span>
          </label>
        </div>
        <select
          class="midi-select"
          value={$midiForwarding.selectedOutputId}
          disabled={!$webMidiState.ready || $webMidiOutputs.length === 0}
          on:change={(event) => setForwarding('selectedOutputId', event.currentTarget.value)}
        >
          <option value="none">None</option>
          {#each $webMidiOutputs as output}
            <option value={output.id}>{output.name}</option>
          {/each}
        </select>
      </div>
    </div>
  </section>

  <section class="tool-section">
    <div class="tool-section-title-row">
      <div class="tool-section-title">Synth</div>
      <button class="midi-action-button" on:click={addSynth}>
        <Add size={16} />
        <span>Add Synth</span>
      </button>
      <button class="midi-icon-button midi-panic" on:click={panic} title="Stop all synth voices">
        <StopFilledAlt size={16} />
      </button>
    </div>

    <div class="midi-synth-list">
      {#if $synthBindings.length === 0}
        <div class="tool-empty">No synth bindings configured.</div>
      {:else}
        {#each $synthBindings as binding (binding.id)}
          <div class="midi-synth-row">
            <label class="midi-switch midi-synth-enable">
              <input
                type="checkbox"
                checked={binding.enabled}
                on:change={(event) => setBinding(binding.id, 'enabled', event.currentTarget.checked)}
              />
              <span></span>
            </label>

            <label class="midi-compact-field">
              <span>Channel</span>
              <select
                value={binding.channel}
                on:change={(event) => setBinding(binding.id, 'channel', event.currentTarget.value)}
              >
                {#each Array.from({ length: 16 }, (_, i) => i + 1) as channel}
                  <option value={channel}>{channel}</option>
                {/each}
              </select>
            </label>

            <div class="midi-grow-field">
              <span>Voice</span>
              <button
                class="midi-voice-button"
                on:click={() => openVoiceSelector(binding)}
                title="Select synth voice"
              >
                {currentPresetName(binding.preset)}
              </button>
            </div>

            <button class="midi-test-button" on:click={() => auditionSynthBinding(binding.id)} title="Audition synth">
              Test
            </button>

            <button class="midi-icon-button" on:click={() => removeSynthBinding(binding.id)} title="Remove synth">
              <Close size={16} />
            </button>
          </div>
        {/each}
      {/if}
    </div>
  </section>

  <section class="tool-section midi-monitor-section">
    <div class="tool-section-title">Monitor & Test Sender</div>
    <div class="midi-monitor-frame">
      <MIDIPanel showHero={false} />
    </div>
  </section>

  {#if voiceSelector}
    <div
      class="midi-modal-backdrop"
      role="button"
      tabindex="0"
      aria-label="Close voice selector"
      on:click={handleBackdropClick}
      on:keydown={(event) => event.key === 'Escape' && closeVoiceSelector()}
    >
      <div class="midi-voice-modal" role="dialog" aria-modal="true">
        <div class="midi-voice-modal-header">
          <div>
            <div class="midi-voice-modal-title">Select Synth Voice</div>
            <div class="midi-voice-modal-subtitle">Click a voice to audition it. Confirm applies it to channel {voiceSelector.channel}.</div>
          </div>
          <button class="midi-icon-button" on:click={closeVoiceSelector} title="Close voice selector">
            <Close size={16} />
          </button>
        </div>

        <div class="midi-voice-tabs">
          {#each synthCategories as category}
            <button
              class="midi-voice-tab"
              class:midi-voice-tab-active={voiceTab === category}
              on:click={() => voiceTab = category}
            >
              {category}
            </button>
          {/each}
        </div>

        <div class="midi-voice-grid">
          {#each visiblePresets as preset}
            <button
              class="midi-voice-option"
              class:midi-voice-option-active={voiceDraft === preset.id}
              on:click={() => auditionVoice(preset)}
            >
              <span class="midi-voice-program">{preset.program + 1}</span>
              <span class="midi-voice-name">{preset.name}</span>
            </button>
          {/each}
        </div>

        <div class="midi-voice-actions">
          <button class="midi-action-button midi-action-muted" on:click={closeVoiceSelector}>Cancel</button>
          <button class="midi-action-button midi-action-confirm" on:click={confirmVoice}>Confirm</button>
        </div>
      </div>
    </div>
  {/if}
</div>

<style>
  .midi-page {
    gap: 14px;
  }
  .midi-hero {
    margin-bottom: 0;
  }
  .midi-status-grid {
    grid-template-columns: repeat(4, minmax(0, 1fr));
  }
  .midi-error {
    padding: 10px 12px;
    border: 1px solid rgba(255, 107, 107, 0.22);
    border-radius: 8px;
    color: #ffb2b2;
    background: rgba(255, 107, 107, 0.08);
    font-size: 0.82rem;
  }
  .midi-bridge-grid {
    display: grid;
    grid-template-columns: repeat(2, minmax(0, 1fr));
    gap: 10px;
  }
  .midi-route-card,
  .midi-synth-row {
    border: 1px solid var(--border);
    border-radius: 8px;
    background: rgba(255, 255, 255, 0.025);
  }
  .midi-route-card {
    display: flex;
    flex-direction: column;
    gap: 12px;
    padding: 12px;
  }
  .midi-route-header {
    display: flex;
    align-items: center;
    justify-content: space-between;
    gap: 12px;
  }
  .midi-route-title {
    font-size: 0.86rem;
    font-weight: 700;
    color: var(--text);
  }
  .midi-route-subtitle {
    margin-top: 2px;
    font-size: 0.74rem;
    color: var(--muted);
  }
  .midi-select,
  .midi-compact-field select,
  .midi-voice-button {
    width: 100%;
    min-height: 34px;
    border: 1px solid var(--border);
    border-radius: 6px;
    background: var(--bg-2);
    color: var(--text);
    padding: 0 9px;
    font: inherit;
    font-size: 0.82rem;
  }
  .midi-voice-button {
    text-align: left;
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
    cursor: pointer;
  }
  .midi-voice-button:hover {
    border-color: rgba(76, 201, 240, 0.45);
    background: rgba(76, 201, 240, 0.08);
  }
  .midi-select:disabled {
    color: var(--muted);
    opacity: 0.55;
  }
  .midi-switch {
    position: relative;
    display: inline-flex;
    width: 42px;
    height: 24px;
    flex-shrink: 0;
  }
  .midi-switch input {
    position: absolute;
    opacity: 0;
    pointer-events: none;
  }
  .midi-switch span {
    position: absolute;
    inset: 0;
    border-radius: 999px;
    border: 1px solid var(--border);
    background: rgba(255, 255, 255, 0.05);
    transition: background 0.12s, border-color 0.12s;
  }
  .midi-switch span::after {
    content: '';
    position: absolute;
    top: 3px;
    left: 3px;
    width: 16px;
    height: 16px;
    border-radius: 50%;
    background: var(--muted);
    transition: transform 0.12s, background 0.12s;
  }
  .midi-switch input:checked + span {
    border-color: rgba(76, 201, 240, 0.55);
    background: rgba(76, 201, 240, 0.16);
  }
  .midi-switch input:checked + span::after {
    transform: translateX(18px);
    background: var(--accent);
  }
  .midi-action-button,
  .midi-test-button,
  .midi-icon-button {
    display: inline-flex;
    align-items: center;
    justify-content: center;
    border: 1px solid var(--border);
    border-radius: 6px;
    background: rgba(255, 255, 255, 0.04);
    color: var(--muted);
    cursor: pointer;
    transition: color 0.12s, border-color 0.12s, background 0.12s;
  }
  .midi-action-button {
    gap: 6px;
    height: 28px;
    padding: 0 9px;
    font-size: 0.76rem;
    font-weight: 700;
  }
  .midi-test-button {
    height: 34px;
    padding: 0 10px;
    font-size: 0.74rem;
    font-weight: 800;
    text-transform: uppercase;
    letter-spacing: 0.04em;
  }
  .midi-icon-button {
    width: 28px;
    height: 28px;
    padding: 0;
  }
  .midi-action-button:hover,
  .midi-test-button:hover,
  .midi-icon-button:hover {
    color: var(--text);
    border-color: rgba(76, 201, 240, 0.45);
    background: rgba(76, 201, 240, 0.08);
  }
  .midi-action-muted {
    color: var(--muted);
  }
  .midi-action-confirm {
    color: #b9ebff;
    border-color: rgba(76, 201, 240, 0.45);
    background: rgba(76, 201, 240, 0.1);
  }
  .midi-panic:hover {
    color: #ffb2b2;
    border-color: rgba(255, 107, 107, 0.45);
    background: rgba(255, 107, 107, 0.08);
  }
  .midi-synth-list {
    display: flex;
    flex-direction: column;
    gap: 8px;
  }
  .midi-synth-row {
    display: grid;
    grid-template-columns: 44px 110px minmax(180px, 1fr) 54px 32px;
    align-items: end;
    gap: 10px;
    padding: 10px;
  }
  .midi-synth-enable {
    align-self: center;
  }
  .midi-compact-field,
  .midi-grow-field {
    display: flex;
    flex-direction: column;
    gap: 4px;
    min-width: 0;
  }
  .midi-compact-field span,
  .midi-grow-field span {
    font-size: 0.68rem;
    color: var(--muted);
    text-transform: uppercase;
    letter-spacing: 0.05em;
  }
  .midi-monitor-section {
    min-height: 360px;
    flex: 1;
  }
  .midi-monitor-frame {
    flex: 1;
    min-height: 0;
    overflow: hidden;
    border: 1px solid var(--border);
    border-radius: 8px;
    background: rgba(255, 255, 255, 0.02);
  }
  .midi-modal-backdrop {
    position: fixed;
    inset: 0;
    z-index: 30;
    display: flex;
    align-items: center;
    justify-content: center;
    padding: 24px;
    background: rgba(0, 0, 0, 0.58);
  }
  .midi-voice-modal {
    display: flex;
    flex-direction: column;
    width: min(880px, 92vw);
    max-height: min(720px, 86vh);
    overflow: hidden;
    border: 1px solid rgba(255, 255, 255, 0.12);
    border-radius: 10px;
    background: #141418;
    box-shadow: 0 18px 48px rgba(0, 0, 0, 0.45);
  }
  .midi-voice-modal-header {
    display: flex;
    align-items: flex-start;
    justify-content: space-between;
    gap: 16px;
    padding: 14px 16px;
    border-bottom: 1px solid var(--border);
  }
  .midi-voice-modal-title {
    font-size: 0.94rem;
    font-weight: 800;
    color: var(--text);
  }
  .midi-voice-modal-subtitle {
    margin-top: 3px;
    font-size: 0.76rem;
    color: var(--muted);
  }
  .midi-voice-tabs {
    display: flex;
    flex-wrap: wrap;
    gap: 6px;
    padding: 10px 12px;
    border-bottom: 1px solid var(--border);
    background: rgba(255, 255, 255, 0.02);
  }
  .midi-voice-tab {
    min-height: 28px;
    border: 1px solid var(--border);
    border-radius: 6px;
    background: rgba(255, 255, 255, 0.035);
    color: var(--muted);
    padding: 0 9px;
    font: inherit;
    font-size: 0.74rem;
    font-weight: 700;
    cursor: pointer;
  }
  .midi-voice-tab:hover,
  .midi-voice-tab-active {
    color: var(--text);
    border-color: rgba(76, 201, 240, 0.42);
    background: rgba(76, 201, 240, 0.1);
  }
  .midi-voice-grid {
    display: grid;
    grid-template-columns: repeat(auto-fill, minmax(190px, 1fr));
    gap: 8px;
    padding: 12px;
    overflow: auto;
  }
  .midi-voice-option {
    display: flex;
    align-items: center;
    gap: 9px;
    min-height: 42px;
    border: 1px solid var(--border);
    border-radius: 7px;
    background: rgba(255, 255, 255, 0.025);
    color: var(--text);
    padding: 0 10px;
    font: inherit;
    cursor: pointer;
    text-align: left;
  }
  .midi-voice-option:hover {
    border-color: rgba(76, 201, 240, 0.36);
    background: rgba(76, 201, 240, 0.08);
  }
  .midi-voice-option-active {
    border-color: rgba(76, 201, 240, 0.6);
    background: rgba(76, 201, 240, 0.14);
  }
  .midi-voice-program {
    width: 28px;
    flex-shrink: 0;
    font-family: var(--mono);
    font-size: 0.72rem;
    color: var(--accent);
    text-align: right;
  }
  .midi-voice-name {
    min-width: 0;
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
    font-size: 0.82rem;
    font-weight: 650;
  }
  .midi-voice-actions {
    display: flex;
    justify-content: flex-end;
    gap: 8px;
    padding: 12px;
    border-top: 1px solid var(--border);
  }
  @media (max-width: 980px) {
    .midi-status-grid,
    .midi-bridge-grid {
      grid-template-columns: repeat(2, minmax(0, 1fr));
    }
  }
  @media (max-width: 720px) {
    .midi-status-grid,
    .midi-bridge-grid {
      grid-template-columns: 1fr;
    }
    .midi-synth-row {
      grid-template-columns: 44px 96px minmax(0, 1fr) 54px 32px;
    }
  }
</style>
