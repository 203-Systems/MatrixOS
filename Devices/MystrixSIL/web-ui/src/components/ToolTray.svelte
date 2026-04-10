<script>
  import {
    Keyboard, Terminal, Activity, Screen, Music,
    Usb, Connect, Meter
  } from 'carbon-icons-svelte'
  import { openTools, toggleTool, deviceTools } from '../stores/tools.js'

  const iconMap = {
    input: Keyboard,
    logs: Terminal,
    runtime: Activity,
    ui: Screen,
    midi: Music,
    hid: Usb,
    serial: Connect,
    usage: Meter,
  }
</script>

<aside class="tool-tray" aria-label="Tool tray">
  {#each deviceTools as tool}
    <button
      class="tray-btn"
      class:tray-active={$openTools.includes(tool.id)}
      on:click={() => toggleTool(tool.id)}
      title={tool.label}
    >
      <svelte:component this={iconMap[tool.id]} size={16} />
      <span class="tray-label">{tool.label}</span>
    </button>
  {/each}
</aside>

<style>
  .tool-tray {
    display: flex;
    flex-direction: column;
    width: 40px;
    background: var(--panel);
    border-left: 1px solid var(--border);
    padding: 6px 0;
    gap: 1px;
    flex-shrink: 0;
    overflow: hidden;
    transition: width 0.15s ease;
    z-index: 5;
  }
  .tool-tray:hover {
    width: 110px;
  }
  .tray-btn {
    display: flex;
    align-items: center;
    gap: 8px;
    padding: 7px 0;
    padding-left: 11px;
    border: none;
    background: none;
    color: var(--muted);
    cursor: pointer;
    font-family: inherit;
    font-size: 0.72rem;
    font-weight: 500;
    white-space: nowrap;
    transition: color 0.12s, background 0.12s;
    border-right: 2px solid transparent;
  }
  .tray-btn:hover {
    color: var(--text);
    background: rgba(255, 255, 255, 0.03);
  }
  .tray-active {
    color: var(--accent);
    border-right-color: var(--accent);
    background: rgba(76, 201, 240, 0.06);
  }
  .tray-label {
    overflow: hidden;
    opacity: 0;
    transition: opacity 0.12s;
  }
  .tool-tray:hover .tray-label {
    opacity: 1;
  }
</style>
