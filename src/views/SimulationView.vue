<script setup>
import { ref, onMounted, computed } from 'vue'
import { Bar } from 'vue-chartjs'
import {
  Chart as ChartJS, BarElement, CategoryScale, LinearScale,
  Tooltip, Legend
} from 'chart.js'
import { fetch_simulation } from '@/services/api.js'

ChartJS.register(BarElement, CategoryScale, LinearScale, Tooltip, Legend)

const simulation = ref(null)
const loading = ref(true)

onMounted(async () => {
  simulation.value = await fetch_simulation()
  loading.value = false
})

const histData = computed(() => {
  if (!simulation.value) return null
  const bins = simulation.value.bins
  const binWidth = simulation.value.bin_width
  const labels = bins.map((_, i) => {
    const lo = (simulation.value.min + i * binWidth)
    return lo.toFixed(1) + '%'
  })
  return {
    labels,
    datasets: [{
      label: 'Frequency',
      data: bins,
      backgroundColor: bins.map((_, i) => {
        const lo = simulation.value.min + i * binWidth
        return lo < 0 ? 'rgba(255,107,107,0.7)' : 'rgba(108,92,231,0.7)'
      }),
      borderColor: bins.map((_, i) => {
        const lo = simulation.value.min + i * binWidth
        return lo < 0 ? '#ff6b6b' : '#6c5ce7'
      }),
      borderWidth: 1,
      borderRadius: 2,
      barPercentage: 1,
      categoryPercentage: 1,
    }]
  }
})

const histOptions = {
  responsive: true,
  maintainAspectRatio: false,
  plugins: {
    legend: { display: false },
    tooltip: {
      backgroundColor: '#1e2130',
      titleColor: '#e4e6f0',
      bodyColor: '#8b8fa3',
      borderColor: '#2a2d3e',
      borderWidth: 1,
      cornerRadius: 8,
    },
  },
  scales: {
    x: {
      grid: { color: 'rgba(42,45,62,0.5)' },
      ticks: { color: '#8b8fa3', maxRotation: 45, autoSkip: true, maxTicksLimit: 15, font: { size: 11 } },
    },
    y: {
      grid: { color: 'rgba(42,45,62,0.5)' },
      ticks: { color: '#8b8fa3', font: { size: 11 } },
    },
  },
}

function fmtPct(val) {
  if (val == null) return '–'
  return (val >= 0 ? '+' : '') + val.toFixed(2) + '%'
}
</script>

<template>
  <div class="max-w-6xl mx-auto px-6 py-8">
    <h1 class="text-2xl font-bold mb-6">Monte Carlo Simulation</h1>

    <div v-if="loading" class="flex items-center justify-center min-h-72 text-text-secondary">
      <div class="spinner"></div>
      Running simulation…
    </div>

    <template v-else-if="simulation">
      <!-- Stats -->
      <div class="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-4 gap-4 mb-4">
        <div class="bg-bg-card border border-border rounded-xl p-6 transition hover:bg-bg-card-hover">
          <div class="text-xs font-semibold uppercase tracking-wider text-text-secondary mb-2">Avg Return</div>
          <div class="text-2xl font-bold" :class="simulation.avg_return >= 0 ? 'text-positive' : 'text-negative'">
            {{ fmtPct(simulation.avg_return) }}
          </div>
        </div>
        <div class="bg-bg-card border border-border rounded-xl p-6 transition hover:bg-bg-card-hover">
          <div class="text-xs font-semibold uppercase tracking-wider text-text-secondary mb-2">VaR 95%</div>
          <div class="text-2xl font-bold text-negative">{{ fmtPct(simulation.var95) }}</div>
        </div>
        <div class="bg-bg-card border border-border rounded-xl p-6 transition hover:bg-bg-card-hover">
          <div class="text-xs font-semibold uppercase tracking-wider text-text-secondary mb-2">VaR 99%</div>
          <div class="text-2xl font-bold text-negative">{{ fmtPct(simulation.var99) }}</div>
        </div>
        <div class="bg-bg-card border border-border rounded-xl p-6 transition hover:bg-bg-card-hover">
          <div class="text-xs font-semibold uppercase tracking-wider text-text-secondary mb-2">Min / Max</div>
          <div class="text-xl font-bold flex items-center gap-1.5">
            <span class="text-negative">{{ fmtPct(simulation.min) }}</span>
            <span class="text-text-secondary font-normal">/</span>
            <span class="text-positive">{{ fmtPct(simulation.max) }}</span>
          </div>
        </div>
      </div>

      <div class="grid grid-cols-1 sm:grid-cols-2 gap-4 mb-6">
        <div class="bg-bg-card border border-border rounded-xl p-6 transition hover:bg-bg-card-hover">
          <div class="text-xs font-semibold uppercase tracking-wider text-text-secondary mb-2">CVaR 95%</div>
          <div class="text-2xl font-bold text-negative">{{ fmtPct(simulation.cvar95) }}</div>
        </div>
        <div class="bg-bg-card border border-border rounded-xl p-6 transition hover:bg-bg-card-hover">
          <div class="text-xs font-semibold uppercase tracking-wider text-text-secondary mb-2">CVaR 99%</div>
          <div class="text-2xl font-bold text-negative">{{ fmtPct(simulation.cvar99) }}</div>
        </div>
      </div>

      <!-- Histogram -->
      <div class="bg-bg-card border border-border rounded-xl p-6 flex flex-col transition hover:bg-bg-card-hover">
        <div class="text-xs font-semibold uppercase tracking-wider text-text-secondary mb-3">Return Distribution</div>
        <div class="min-h-90 relative">
          <Bar v-if="histData" :data="histData" :options="histOptions" />
        </div>
      </div>
    </template>

    <div v-else class="flex items-center justify-center min-h-72 text-text-secondary">No simulation data available.</div>
  </div>
</template>
