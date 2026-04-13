<script>
  import { Dashboard, Connect } from 'carbon-icons-svelte'

  export let active = 'device'
  let expanded = false

  const sections = [
    { id: 'device',     label: 'Device',     icon: Dashboard },
    { id: 'connection', label: 'Connection', icon: Connect },
  ]
</script>

<nav
  class="left-nav"
  class:expanded
  aria-label="Section navigation"
  on:mouseenter={() => (expanded = true)}
  on:mouseleave={() => (expanded = false)}
>
  <div class="nav-group">
    {#each sections as sec}
      <button
        class="nav-item"
        class:nav-active={active === sec.id}
        on:click={() => (active = sec.id)}
        title={sec.label}
      >
        <svelte:component this={sec.icon} size={20} />
        <span class="nav-label">{sec.label}</span>
      </button>
    {/each}
  </div>
</nav>

<style>
  .left-nav {
    display: flex;
    flex-direction: column;
    justify-content: space-between;
    width: 60px;
    background: var(--panel);
    border-right: 1px solid var(--border);
    padding: 8px 0;
    flex-shrink: 0;
    overflow: hidden;
    transition: width 0.22s cubic-bezier(0.2, 0.8, 0.2, 1);
    z-index: 5;
  }
  .left-nav.expanded {
    width: 168px;
  }
  .nav-group {
    display: flex;
    flex-direction: column;
    gap: 2px;
  }
  .nav-item {
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
    border-left: 2px solid transparent;
    overflow: hidden;
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
  .left-nav.expanded .nav-label {
    opacity: 1;
    max-width: 128px;
    transform: translateX(0);
    transition-delay: 0.05s, 0.05s, 0.05s;
  }
</style>
