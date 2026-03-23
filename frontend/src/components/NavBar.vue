<script setup>
import { RouterLink } from 'vue-router'
</script>

<template>
  <nav class="sticky top-0 z-50 bg-bg-primary/85 backdrop-blur-xl border-b border-border">
    <div class="max-w-6xl mx-auto flex items-center justify-between px-6 h-15">
      <RouterLink to="/" class="flex items-center gap-2 font-bold text-lg text-text-primary no-underline">
        <span class="text-xl text-accent">◈</span>
        <span>Meridian</span>
      </RouterLink>

      <div class="flex gap-1">
        <RouterLink to="/" class="px-4 py-2 rounded-lg text-sm font-medium text-text-secondary no-underline transition-all hover:text-text-primary hover:bg-accent-muted [&.router-link-exact-active]:text-accent [&.router-link-exact-active]:bg-accent-muted">
          My Portfolio
        </RouterLink>
        <RouterLink to="/simulation" class="px-4 py-2 rounded-lg text-sm font-medium text-text-secondary no-underline transition-all hover:text-text-primary hover:bg-accent-muted [&.router-link-exact-active]:text-accent [&.router-link-exact-active]:bg-accent-muted">
          Simulation
        </RouterLink>
        <RouterLink to="/explorer" class="px-4 py-2 rounded-lg text-sm font-medium text-text-secondary no-underline transition-all hover:text-text-primary hover:bg-accent-muted [&.router-link-exact-active]:text-accent [&.router-link-exact-active]:bg-accent-muted">
          Asset Explorer
        </RouterLink>
        <RouterLink to="/backtest" class="px-4 py-2 rounded-lg text-sm font-medium text-text-secondary no-underline transition-all hover:text-text-primary hover:bg-accent-muted [&.router-link-exact-active]:text-accent [&.router-link-exact-active]:bg-accent-muted">
          Backtest
        </RouterLink>
      </div>
    </div>
  </nav>
</template>
