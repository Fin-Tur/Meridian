<script setup>
const model = defineModel({ type: String, default: '$' })

function toggle() {
  model.value = model.value === '$' ? '%' : '$'
}
</script>

<template>
  <button
    @click="toggle"
    class="px-2.5 py-1 text-xs font-semibold rounded-md border transition-colors cursor-pointer"
    :class="model === '$'
      ? 'bg-accent-muted text-accent border-accent/30'
      : 'bg-bg-secondary text-text-secondary border-border hover:text-text-primary'"
  >
    {{ model }}
  </button>
</template>
