<script setup>
import { RouterLink } from 'vue-router'
</script>

<template>
  <nav class="navbar">
    <div class="navbar-inner">
      <RouterLink to="/" class="navbar-brand">
        <span class="brand-icon">◈</span>
        <span class="brand-text">Meridian</span>
      </RouterLink>

      <div class="navbar-links">
        <RouterLink to="/" class="nav-link">My Portfolio</RouterLink>
        <RouterLink to="/simulation" class="nav-link">Simulation</RouterLink>
        <RouterLink to="/explorer" class="nav-link">Asset Explorer</RouterLink>
      </div>
    </div>
  </nav>
</template>

<style scoped>
.navbar {
  position: sticky;
  top: 0;
  z-index: 100;
  background: rgba(15, 17, 23, 0.85);
  backdrop-filter: blur(16px);
  border-bottom: 1px solid var(--border-color);
}

.navbar-inner {
  max-width: 1200px;
  margin: 0 auto;
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 0 1.5rem;
  height: 60px;
}

.navbar-brand {
  display: flex;
  align-items: center;
  gap: 0.5rem;
  font-weight: 700;
  font-size: 1.15rem;
  color: var(--text-primary);
  text-decoration: none;
}

.brand-icon {
  font-size: 1.4rem;
  color: var(--accent);
}

.navbar-links {
  display: flex;
  gap: 0.25rem;
}

.nav-link {
  padding: 0.5rem 1rem;
  border-radius: var(--radius-sm);
  font-size: 0.9rem;
  font-weight: 500;
  color: var(--text-secondary);
  text-decoration: none;
  transition: all var(--transition);
}

.nav-link:hover {
  color: var(--text-primary);
  background: var(--accent-muted);
}

.nav-link.router-link-exact-active {
  color: var(--accent);
  background: var(--accent-muted);
}
</style>
