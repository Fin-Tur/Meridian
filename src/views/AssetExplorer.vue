<script setup>
import { ref, computed } from 'vue'
import { Bar } from 'vue-chartjs'
import {
  Chart as ChartJS, BarElement, CategoryScale, LinearScale,
  Tooltip, Legend
} from 'chart.js'
import { fetch_asset } from '@/services/api.js'

ChartJS.register(BarElement, CategoryScale, LinearScale, Tooltip, Legend)

const symbol = ref('')
const asset = ref(null)
const loading = ref(false)
const searched = ref(false)

async function search() {
  const sym = symbol.value.trim().toUpperCase()
  if (!sym) return
  loading.value = true
  searched.value = true
  asset.value = await fetch_asset(sym)
  loading.value = false
}

const histData = computed(() => {
  if (!asset.value?.adj_closes || asset.value.adj_closes.length < 2) return null
  // Compute daily log returns
  const closes = asset.value.adj_closes
  const returns = []
  for (let i = 1; i < closes.length; i++) {
    if (closes[i - 1] > 0) {
      returns.push(Math.log(closes[i] / closes[i - 1]) * 100)
    }
  }
  if (!returns.length) return null

  // Build histogram bins
  const min = Math.min(...returns)
  const max = Math.max(...returns)
  const binCount = 40
  const binWidth = (max - min) / binCount || 1
  const bins = new Array(binCount).fill(0)
  for (const r of returns) {
    const idx = Math.min(Math.floor((r - min) / binWidth), binCount - 1)
    bins[idx]++
  }

  const labels = bins.map((_, i) => (min + i * binWidth).toFixed(2) + '%')

  return {
    labels,
    datasets: [{
      label: 'Frequency',
      data: bins,
      backgroundColor: bins.map((_, i) => {
        const lo = min + i * binWidth
        return lo < 0 ? 'rgba(255,107,107,0.7)' : 'rgba(0,206,201,0.7)'
      }),
      borderColor: bins.map((_, i) => {
        const lo = min + i * binWidth
        return lo < 0 ? '#ff6b6b' : '#00cec9'
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
  return (val >= 0 ? '+' : '') + val.toFixed(4)
}
</script>

<template>
  <div class="max-w-6xl mx-auto px-6 py-8">
    <h1 class="text-2xl font-bold mb-6">Asset Explorer</h1>

    <!-- Search -->
    <div class="flex gap-3 mb-8">
      <input
        v-model="symbol"
        placeholder="Enter ticker symbol (e.g. AAPL)"
        @keydown.enter="search"
        class="flex-1 max-w-sm bg-bg-secondary border border-border text-text-primary px-4 py-2.5 rounded-lg text-sm outline-none transition focus:border-accent"
      />
      <button @click="search" :disabled="loading"
              class="px-5 py-2.5 bg-accent text-white font-semibold text-sm rounded-lg transition hover:bg-accent-hover cursor-pointer disabled:opacity-50">
        {{ loading ? 'Loading…' : 'Search' }}
      </button>
    </div>

    <div v-if="loading" class="flex items-center justify-center min-h-72 text-text-secondary">
      <div class="spinner"></div>
      Fetching asset data…
    </div>

    <template v-else-if="asset">
      <h2 class="text-xl font-bold mb-5 flex items-center gap-3">
        {{ asset.symbol }}
        <span class="text-xs font-semibold px-2.5 py-1 rounded-full bg-accent-muted text-accent">{{ asset.currency }}</span>
      </h2>

      <!-- Stats grid -->
      <div class="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-4 gap-4 mb-4">
        <div class="bg-bg-card border border-border rounded-xl p-6 transition hover:bg-bg-card-hover">
          <div class="text-xs font-semibold uppercase tracking-wider text-text-secondary mb-2">Volatility</div>
          <div class="text-2xl font-bold">{{ (asset.volatility * 100).toFixed(2) }}%</div>
        </div>
        <div class="bg-bg-card border border-border rounded-xl p-6 transition hover:bg-bg-card-hover">
          <div class="text-xs font-semibold uppercase tracking-wider text-text-secondary mb-2">Avg Log Return</div>
          <div class="text-2xl font-bold" :class="asset.avg_log_return >= 0 ? 'text-positive' : 'text-negative'">
            {{ fmtPct(asset.avg_log_return) }}
          </div>
        </div>
        <div class="bg-bg-card border border-border rounded-xl p-6 transition hover:bg-bg-card-hover">
          <div class="text-xs font-semibold uppercase tracking-wider text-text-secondary mb-2">Sharpe Ratio</div>
          <div class="text-2xl font-bold">{{ asset.sharpe_ratio?.toFixed(3) ?? '–' }}</div>
        </div>
        <div class="bg-bg-card border border-border rounded-xl p-6 transition hover:bg-bg-card-hover">
          <div class="text-xs font-semibold uppercase tracking-wider text-text-secondary mb-2">Max Drawdown</div>
          <div class="text-2xl font-bold text-negative">{{ (asset.max_drawdown * 100).toFixed(2) }}%</div>
        </div>
      </div>

      <div class="grid grid-cols-1 sm:grid-cols-3 gap-4 mb-6">
        <div class="bg-bg-card border border-border rounded-xl p-6 transition hover:bg-bg-card-hover">
          <div class="text-xs font-semibold uppercase tracking-wider text-text-secondary mb-2">YTD Return</div>
          <div class="text-2xl font-bold" :class="asset.ytd_returns >= 0 ? 'text-positive' : 'text-negative'">
            {{ (asset.ytd_returns * 100).toFixed(2) }}%
          </div>
        </div>
        <div class="bg-bg-card border border-border rounded-xl p-6 transition hover:bg-bg-card-hover">
          <div class="text-xs font-semibold uppercase tracking-wider text-text-secondary mb-2">Skewness</div>
          <div class="text-2xl font-bold">{{ asset.skewness?.toFixed(4) ?? '–' }}</div>
        </div>
        <div class="bg-bg-card border border-border rounded-xl p-6 transition hover:bg-bg-card-hover">
          <div class="text-xs font-semibold uppercase tracking-wider text-text-secondary mb-2">Kurtosis</div>
          <div class="text-2xl font-bold">{{ asset.kurtosis?.toFixed(4) ?? '–' }}</div>
        </div>
      </div>

      <!-- Histogram of daily returns -->
      <div v-if="histData" class="bg-bg-card border border-border rounded-xl p-6 flex flex-col transition hover:bg-bg-card-hover">
        <div class="text-xs font-semibold uppercase tracking-wider text-text-secondary mb-3">Daily Return Distribution</div>
        <div class="min-h-90 relative">
          <Bar :data="histData" :options="histOptions" />
        </div>
      </div>
    </template>

    <div v-else-if="searched && !loading" class="flex items-center justify-center min-h-72 text-text-secondary">
      No data found for this symbol.
    </div>
  </div>
</template>
