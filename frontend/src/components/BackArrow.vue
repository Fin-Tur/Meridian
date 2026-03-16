<template>
  <button
    @click="$emit('back')"
    class="inline-flex items-center gap-1.5 px-3 py-2 rounded-lg text-sm font-medium text-text-secondary transition-all hover:text-text-primary hover:bg-accent-muted cursor-pointer border-none bg-transparent"
  >
    <svg xmlns="http://www.w3.org/2000/svg" width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5" stroke-linecap="round" stroke-linejoin="round">
      <polyline points="15 18 9 12 15 6" />
    </svg>
    <span>Back</span>
  </button>
</template>

<script setup>
defineEmits(['back'])
</script>
