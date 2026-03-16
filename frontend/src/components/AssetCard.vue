<script setup>
const props = defineProps({
  symbol: { type: String, required: true },
  name: { type: String, default: '' },
  price: { type: Number, default: null },
  currency: { type: String, default: 'USD' },
  ytdReturn: { type: Number, default: null },
})

const emit = defineEmits(['select'])

function color(val) {
  return val >= 0 ? 'text-positive' : 'text-negative'
}
</script>

<template>
  <div @click="emit('select', symbol)"
       class="bg-bg-card border border-border rounded-xl p-5 cursor-pointer transition hover:bg-bg-card-hover hover:border-accent group">

    <div class="flex items-center gap-2 mb-3">
      <span class="text-base font-bold text-text-primary group-hover:text-accent transition">{{ symbol }}</span>
      <span v-if="name" class="text-xs text-text-secondary truncate">{{ name }}</span>
    </div>

    <div v-if="price != null" class="text-lg font-semibold mb-2">
      {{ currency === 'USD' ? '$' : currency }}{{ price.toLocaleString(undefined, { minimumFractionDigits: 2, maximumFractionDigits: 2 }) }}
    </div>

    <div class="flex gap-4 text-xs font-medium">
      <span v-if="ytdReturn != null" :class="color(ytdReturn)">YTD {{ ytdReturn.toFixed(2) }}%</span>
    </div>
  </div>
</template>
