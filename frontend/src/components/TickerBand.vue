<script setup>
import { formatCurrency } from '@/services/utils';
const props = defineProps({
  assets: { type: Array, default: () => [] },
})

function color(val) {
  return val >= 0 ? '#22c55e' : '#ef4444'
}
</script>

<template>
  <div v-if="assets.length" class="ticker-wrapper overflow-hidden border-y border-white/5 py-2 ">
    <div class="ticker-track flex gap-10 whitespace-nowrap">
      <!-- duplicate for seamless loop -->
      <template v-for="pass in 2" :key="pass">
        <span
          v-for="a in assets"
          :key="`${pass}-${a.symbol}`"
          class="inline-flex items-center gap-2 text-xs font-mono"
        >
          <span class="font-bold text-text-primary tracking-wide">{{ a.symbol }}</span>
          <span class="text-text-secondary">
            {{ formatCurrency(a.currency, a.adj_closes?.[a.adj_closes.length - 1] ?? 0) }}
          </span>
          <span :style="{ color: color(a.ytd_return) }" class="font-semibold">
            {{ a.ytd_return >= 0 ? '+' : '' }}{{ a.ytd_return?.toFixed(2) }}%
          </span>
          <span class="text-white/10 mx-2">◆</span>
        </span>
      </template>
    </div>
  </div>
</template>

<style scoped>
.ticker-wrapper {
  mask-image: linear-gradient(to right, transparent, black 8%, black 92%, transparent);
}

.ticker-track {
  animation: ticker-scroll 35s linear infinite;
  width: max-content;
}

.ticker-wrapper:hover .ticker-track {
  animation-play-state: paused;
}

@keyframes ticker-scroll {
  0%   { transform: translateX(0); }
  100% { transform: translateX(-50%); }
}
</style>
