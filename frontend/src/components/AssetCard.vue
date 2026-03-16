<script setup>
import { ref, onMounted } from 'vue'
import { formatCurrency } from '@/services/utils';

const props = defineProps({
  symbol: { type: String, required: true },
  name: { type: String, default: '' },
  price: { type: Number, default: null },
  currency: { type: String, default: 'USD' },
  ytdReturn: { type: Number, default: null },
  sparklineData: { type: Array, default: () => [] },
})

const emit = defineEmits(['select'])

function textColor(val) {
  return val >= 0 ? '#22c55e' : '#ef4444'
}

// Animated counter
const displayPrice = ref(0)
onMounted(() => {
  if (props.price == null) return
  const target = props.price
  const steps = 40
  const duration = 700
  const increment = target / steps
  let current = 0
  const interval = setInterval(() => {
    current += increment
    if (current >= target) {
      displayPrice.value = target
      clearInterval(interval)
    } else {
      displayPrice.value = current
    }
  }, duration / steps)
})

// Sparkline SVG path builder
function buildPath(data) {
  if (!data || data.length < 2) return ''
  const w = 100, h = 32
  const min = Math.min(...data)
  const max = Math.max(...data)
  const range = max - min || 1
  const pts = data.map((v, i) => {
    const x = (i / (data.length - 1)) * w
    const y = h - ((v - min) / range) * h
    return `${x.toFixed(2)},${y.toFixed(2)}`
  })
  return 'M ' + pts.join(' L ')
}
</script>

<template>
  <div @click="emit('select', symbol)" class="asset-card rounded-xl p-5 cursor-pointer group">
    <div class="flex items-center gap-2 mb-2">
      <span class="text-sm font-bold text-text-primary group-hover:text-accent transition-colors">{{ symbol }}</span>
      <span v-if="name" class="text-xs text-text-secondary truncate">{{ name }}</span>
    </div>

    <div v-if="price != null" class="text-lg font-semibold mb-1">
      {{ formatCurrency(currency, displayPrice) }}
    </div>

    <div class="flex gap-4 text-xs font-medium mb-3">
      <span v-if="ytdReturn != null" :style="{ color: textColor(ytdReturn) }">
        {{ ytdReturn >= 0 ? '+' : '' }}{{ ytdReturn.toFixed(2) }}%
      </span>
    </div>

    <!-- Sparkline -->
    <svg v-if="sparklineData.length > 1" viewBox="0 0 100 32" class="w-full h-8" preserveAspectRatio="none">
      <defs>
        <linearGradient :id="`sg-${symbol}`" x1="0" y1="0" x2="0" y2="1">
          <stop offset="0%" :stop-color="textColor(ytdReturn ?? 0)" stop-opacity="0.3"/>
          <stop offset="100%" :stop-color="textColor(ytdReturn ?? 0)" stop-opacity="0"/>
        </linearGradient>
      </defs>
      <path :d="buildPath(sparklineData) + ' L 100,32 L 0,32 Z'"
            :fill="`url(#sg-${symbol})`" />
      <path :d="buildPath(sparklineData)"
            :stroke="textColor(ytdReturn ?? 0)"
            stroke-width="1.5" fill="none" stroke-linecap="round" stroke-linejoin="round" />
    </svg>
  </div>
</template>

<style scoped>
.asset-card {
  background: rgba(255, 255, 255, 0.03);
  backdrop-filter: blur(12px);
  border: 1px solid rgba(255, 255, 255, 0.08);
  box-shadow: 0 0 20px rgba(108, 92, 231, 0.15);
  transition: transform 0.2s ease, box-shadow 0.2s ease, border-color 0.2s ease;
}

.asset-card:hover {
  transform: translateY(-4px);
  box-shadow: 0 0 35px rgba(108, 92, 231, 0.4);
  border-color: rgba(108, 92, 231, 0.5);
}
</style>
