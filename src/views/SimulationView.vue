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
  <div class="page">
    <h1 class="page-title">Monte Carlo Simulation</h1>

    <div v-if="loading" class="loading">
      <div class="spinner"></div>
      Running simulation…
    </div>

    <template v-else-if="simulation">
      <!-- Stats -->
      <div class="grid-4 stat-row">
        <div class="card">
          <div class="card-title">Avg Return</div>
          <div class="stat-value" :class="simulation.avg_return >= 0 ? 'positive' : 'negative'">
            {{ fmtPct(simulation.avg_return) }}
          </div>
        </div>
        <div class="card">
          <div class="card-title">VaR 95%</div>
          <div class="stat-value negative">{{ fmtPct(simulation.var95) }}</div>
        </div>
        <div class="card">
          <div class="card-title">VaR 99%</div>
          <div class="stat-value negative">{{ fmtPct(simulation.var99) }}</div>
        </div>
        <div class="card">
          <div class="card-title">Min / Max</div>
          <div class="stat-minmax">
            <span class="negative">{{ fmtPct(simulation.min) }}</span>
            <span class="divider">/</span>
            <span class="positive">{{ fmtPct(simulation.max) }}</span>
          </div>
        </div>
      </div>

      <div class="grid-2 cvar-row">
        <div class="card">
          <div class="card-title">CVaR 95%</div>
          <div class="stat-value negative">{{ fmtPct(simulation.cvar95) }}</div>
        </div>
        <div class="card">
          <div class="card-title">CVaR 99%</div>
          <div class="stat-value negative">{{ fmtPct(simulation.cvar99) }}</div>
        </div>
      </div>

      <!-- Histogram -->
      <div class="card hist-card">
        <div class="card-title">Return Distribution</div>
        <div class="hist-wrapper">
          <Bar v-if="histData" :data="histData" :options="histOptions" />
        </div>
      </div>
    </template>

    <div v-else class="loading">No simulation data available.</div>
  </div>
</template>

<style scoped>
.stat-row {
  margin-bottom: 1rem;
}

.cvar-row {
  margin-bottom: 1.5rem;
}

.stat-minmax {
  font-size: 1.3rem;
  font-weight: 700;
  display: flex;
  align-items: center;
  gap: 0.4rem;
}

.stat-minmax .positive {
  color: var(--green);
}

.stat-minmax .negative {
  color: var(--red);
}

.stat-minmax .divider {
  color: var(--text-secondary);
  font-weight: 400;
}

.hist-card {
  display: flex;
  flex-direction: column;
}

.hist-wrapper {
  min-height: 360px;
  position: relative;
}
</style>
