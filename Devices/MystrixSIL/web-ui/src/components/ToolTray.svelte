<script>
  import {
    Keyboard, Terminal, Activity, Screen, Music,
    Usb, Connect, Meter, Application, Plug, Api,
    Compass, BatteryFull, DataBase, Restart
  } from 'carbon-icons-svelte'
  import { openTools, toggleTool, deviceTools } from '../stores/tools.js'
  import { doReboot } from '../stores/wasm.js'
  let expanded = false

  const iconMap = {
    application: Application,
    api: Api,
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

<aside
  class="tool-tray"
  class:expanded
  aria-label="Tool tray"
  on:mouseenter={() => (expanded = true)}
  on:mouseleave={() => (expanded = false)}
>
  {#each deviceTools as tool}
    <button
      class="tray-btn"
      class:tray-active={$openTools.includes(tool.id)}
      on:click={() => toggleTool(tool.id)}
      title={tool.label}
    >
      <svelte:component this={iconMap[tool.id]} size={22} />
      <span class="tray-label">{tool.label}</span>
    </button>
  {/each}

  <div class="tray-footer">
    <button
      class="tray-btn tray-reset"
      on:click={doReboot}
      title="Reset emulator"
    >
      <Restart size={22} />
      <span class="tray-label">Reset</span>
    </button>
  </div>
</aside>

<style>
  .tool-tray {
    display: flex;
    flex-direction: column;
    width: 60px;
    background: var(--panel);
    border-left: 1px solid var(--border);
    padding: 6px 0;
    gap: 1px;
    flex-shrink: 0;
    overflow: hidden;
    overflow-y: hidden;
    transition: width 0.22s cubic-bezier(0.2, 0.8, 0.2, 1);
    z-index: 5;
    scrollbar-width: none;
  }
  .tool-tray::-webkit-scrollbar { display: none; }
  .tool-tray.expanded {
    width: 144px;
  }
  .tray-btn {
    display: flex;
    align-items: center;
    justify-content: flex-start;
    gap: 10px;
    width: 100%;
    min-height: 46px;
    padding: 10px 0 10px 18px;
    border: none;
    background: none;
    color: var(--muted);
    cursor: pointer;
    font-family: inherit;
    font-size: 0.82rem;
    font-weight: 500;
    white-space: nowrap;
    transition: color 0.12s, background 0.12s;
    border-right: 2px solid transparent;
    flex-shrink: 0;
    overflow: hidden;
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
  .tray-footer {
    margin-top: auto;
    padding-top: 8px;
    border-top: 1px solid rgba(255, 255, 255, 0.05);
    display: flex;
    flex-direction: column;
    gap: 1px;
    flex-shrink: 0;
  }
  .tray-reset {
    color: #ffb48a;
  }
  .tray-reset:hover {
    color: #ffd4bc;
    background: rgba(255, 148, 89, 0.08);
  }
  .tray-label {
    display: inline-block;
    overflow: hidden;
    opacity: 0;
    max-width: 0;
    transform: translateX(-6px);
    white-space: nowrap;
    transition:
      opacity 0.14s ease,
      max-width 0.24s cubic-bezier(0.2, 0.8, 0.2, 1),
      transform 0.22s cubic-bezier(0.2, 0.8, 0.2, 1);
    transition-delay: 0s, 0s, 0s;
  }
  .tool-tray.expanded .tray-label {
    opacity: 1;
    max-width: 120px;
    transform: translateX(0);
    transition-delay: 0.05s, 0.05s, 0.05s;
  }
</style>
