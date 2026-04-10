<script>
  import { Dashboard, Keyboard, Terminal, Activity } from 'carbon-icons-svelte'

  export let active = 'device'

  const sections = [
    { id: 'device',  label: 'Device',  icon: Dashboard },
    { id: 'input',   label: 'Input',   icon: Keyboard },
    { id: 'logs',    label: 'Logs',    icon: Terminal },
    { id: 'runtime', label: 'Runtime', icon: Activity },
  ]
</script>

<nav class="left-nav" aria-label="Section navigation">
  {#each sections as sec}
    <button
      class="nav-item"
      class:nav-active={active === sec.id}
      on:click={() => (active = sec.id)}
      title={sec.label}
    >
      <svelte:component this={sec.icon} size={18} />
      <span class="nav-label">{sec.label}</span>
    </button>
  {/each}
</nav>

<style>
  .left-nav {
    display: flex;
    flex-direction: column;
    width: 56px;
    background: var(--panel);
    border-right: 1px solid var(--border);
    padding: 8px 0;
    gap: 2px;
    flex-shrink: 0;
    overflow: hidden;
    transition: width 0.15s ease;
    z-index: 5;
  }
  .left-nav:hover {
    width: 140px;
  }
  .nav-item {
    display: flex;
    align-items: center;
    gap: 10px;
    padding: 10px 0;
    padding-left: 18px;
    border: none;
    background: none;
    color: var(--muted);
    cursor: pointer;
    font-family: inherit;
    font-size: 0.78rem;
    font-weight: 500;
    white-space: nowrap;
    transition: color 0.12s, background 0.12s;
    border-left: 2px solid transparent;
  }
  .nav-item:hover {
    color: var(--text);
    background: rgba(255, 255, 255, 0.03);
  }
  .nav-active {
    color: var(--accent);
    border-left-color: var(--accent);
    background: rgba(76, 201, 240, 0.06);
  }
  .nav-label {
    overflow: hidden;
    opacity: 0;
    transition: opacity 0.12s;
  }
  .left-nav:hover .nav-label {
    opacity: 1;
  }
</style>
