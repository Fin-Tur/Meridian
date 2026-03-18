<script setup>
import { ref, computed } from 'vue'
const props = defineProps({
  content: {
    type: String,
    required: true
  }
})
const showTooltip = ref(false)
</script>
<template>
    <div class="relative flex items-center"
                 @mouseenter="showTooltip = true" @mouseleave="showTooltip = false">
              <svg xmlns="http://www.w3.org/2000/svg" class="w-3.5 h-3.5 text-text-secondary/60 hover:text-text-secondary cursor-default transition-colors" viewBox="0 0 24 24" fill="currentColor">
                <path fill-rule="evenodd" d="M2.25 12c0-5.385 4.365-9.75 9.75-9.75s9.75 4.365 9.75 9.75-4.365 9.75-9.75 9.75S2.25 17.385 2.25 12zm8.706-1.442c1.146-.573 2.437.463 2.126 1.706l-.709 2.836.042-.02a.75.75 0 01.67 1.34l-.04.022c-1.147.573-2.438-.463-2.127-1.706l.71-2.836-.042.02a.75.75 0 11-.671-1.34l.041-.022zM12 9a.75.75 0 100-1.5A.75.75 0 0012 9z" clip-rule="evenodd" />
              </svg>
              <Transition name="tooltip-fade">
                <div v-if="showTooltip"
                     class="absolute z-10 left-1/2 -translate-x-1/2 bottom-full mb-2 px-3 py-2 text-sm text-text-primary bg-bg-secondary border border-border rounded-lg shadow-lg w-48 text-center whitespace-normal pointer-events-none">
                  {{ content }}
                </div>
              </Transition>
            </div>
    </template>

<style scoped>
.tooltip-fade-enter-active,
.tooltip-fade-leave-active {
  transition: opacity 0.15s ease;
}
.tooltip-fade-enter-from,
.tooltip-fade-leave-to {
  opacity: 0;
}
</style>