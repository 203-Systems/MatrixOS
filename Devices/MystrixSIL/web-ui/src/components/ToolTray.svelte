<script>
  import {
    Keyboard, Terminal, Activity, Screen, Music,
    Usb, Connect, Meter, Application, Plug,
    Compass, BatteryFull, DataBase
  } from 'carbon-icons-svelte'
  import { openTools, toggleTool, deviceTools } from '../stores/tools.js'

  const iconMap = {
    application: Application,
    input: Keyboard,
    logs: Terminal,
    runtime: Activity,
    ui: Screen,
    midi: Music,
    hid: Usb,
    serial: Connect,
    usage: Meter,
    usb: Plug,
    gyro: Compass,
    battery: BatteryFull,
    storage: DataBase,
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
      <svelte:component this={iconMap[tool.id]} size={20} />
      <span class="tray-label">{tool.label}</span>
    </button>
  {/each}
</aside>

<style>
  .tool-tray {
    display: flex;
    flex-direction: column;
    width: 48px;
    background: var(--panel);
    border-left: 1px solid var(--border);
    padding: 6px 0;
    gap: 1px;
    flex-shrink: 0;
    overflow: hidden;
    overflow-y: auto;
    transition: width 0.15s ease;
    z-index: 5;
    scrollbar-width: none;
  }
  .tool-tray::-webkit-scrollbar { display: none; }
  .tool-tray:hover {
    width: 130px;
  }
  .tray-btn {
    display: flex;
    align-items: center;
    gap: 8px;
    padding: 8px 0;
    padding-left: 13px;
    border: none;
    background: none;
    color: var(--muted);
    cursor: pointer;
    font-family: inherit;
    font-size: 0.78rem;
    font-weight: 500;
    white-space: nowrap;
    transition: color 0.12s, background 0.12s;
    border-right: 2px solid transparent;
    flex-shrink: 0;
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
