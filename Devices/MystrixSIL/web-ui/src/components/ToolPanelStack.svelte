<script>
  import { Close } from 'carbon-icons-svelte'
  import { openTools, closeTool, deviceTools } from '../stores/tools.js'
  import InputPanel from './InputPanel.svelte'
  import LogsPanel from './LogsPanel.svelte'
  import RuntimePanel from './RuntimePanel.svelte'
  import UIPanel from './tools/UIPanel.svelte'
  import MIDIPanel from './tools/MIDIPanel.svelte'
  import HIDPanel from './tools/HIDPanel.svelte'
  import SerialPanel from './tools/SerialPanel.svelte'
  import UsagePanel from './tools/UsagePanel.svelte'

  const panelMap = {
    input: InputPanel,
    logs: LogsPanel,
    runtime: RuntimePanel,
    ui: UIPanel,
    midi: MIDIPanel,
    hid: HIDPanel,
    serial: SerialPanel,
    usage: UsagePanel,
  }

  function getLabel(id) {
    const tool = deviceTools.find(t => t.id === id)
    return tool ? tool.label : id
  }
</script>

{#if $openTools.length > 0}
  <div class="panel-stack">
    {#each $openTools as toolId (toolId)}
      <div class="panel-slot">
        <div class="panel-header">
          <span class="panel-title">{getLabel(toolId)}</span>
          <button class="panel-close" on:click={() => closeTool(toolId)} title="Close {getLabel(toolId)}">
            <Close size={14} />
          </button>
        </div>
        <div class="panel-body">
          <svelte:component this={panelMap[toolId]} />
        </div>
      </div>
    {/each}
  </div>
{/if}

<style>
  .panel-stack {
    display: flex;
    flex-direction: column;
    width: 380px;
    min-width: 280px;
    max-width: 50vw;
    border-left: 1px solid var(--border);
    background: var(--bg-1);
    overflow: hidden;
  }
  .panel-slot {
    flex: 1;
    display: flex;
    flex-direction: column;
    min-height: 120px;
    overflow: hidden;
    border-bottom: 1px solid var(--border);
  }
  .panel-slot:last-child {
    border-bottom: none;
  }
  .panel-header {
    display: flex;
    align-items: center;
    justify-content: space-between;
    padding: 4px 10px;
    background: var(--panel);
    border-bottom: 1px solid var(--border);
    flex-shrink: 0;
  }
  .panel-title {
    font-size: 0.72rem;
    font-weight: 600;
    text-transform: uppercase;
    letter-spacing: 0.06em;
    color: var(--muted);
  }
  .panel-close {
    background: none;
    border: none;
    color: var(--muted);
    cursor: pointer;
    padding: 2px;
    display: inline-flex;
    align-items: center;
    border-radius: 3px;
  }
  .panel-close:hover {
    color: var(--text);
    background: rgba(255, 255, 255, 0.06);
  }
  .panel-body {
    flex: 1;
    min-height: 0;
    overflow: hidden;
  }
</style>
