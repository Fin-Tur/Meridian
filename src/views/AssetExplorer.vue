<script setup>
import { ref, computed } from 'vue'
import { Bar } from 'vue-chartjs'
import {
  Chart as ChartJS, BarElement, CategoryScale, LinearScale,
  Tooltip, Legend
} from 'chart.js'
import { fetch_asset } from '@/services/api.js'
import InfoCard from '@/components/InfoCard.vue'
import SearchBar from '@/components/SearchBar.vue'
import Histogram from '@/components/Histogram.vue'

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
  // Compute daily log returns - IN BACKEND IMPL
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

</script>

<template>
  <div class="max-w-6xl mx-auto px-6 py-8">
    <h1 class="text-2xl font-bold mb-6">Asset Explorer</h1>

    <!-- Search -->
    <div class="flex gap-3 mb-8">
      <SearchBar v-model="symbol" :text="'Enter asset symbol (e.g. AAPL)'" @search="search" />
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
        <InfoCard title="Volatility" :val="asset.volatility" :type="'percentile'" :decimals="2" />
        <InfoCard title="Avg Log Return" :val="asset.avg_log_return" :type="'beneficial'" :decimals="4" />
        <InfoCard title="Sharpe Ratio" :val="asset.sharpe_ratio" :decimals="3" />
        <InfoCard title="Max Drawdown" :val="asset.max_drawdown" :type="'beneficial-percentile'" :decimals="2" />
      </div>

      <div class="grid grid-cols-1 sm:grid-cols-3 gap-4 mb-6">
        <InfoCard title="YTD Returns" :val="asset.ytd_returns" :type="'beneficial-percentile'" :decimals="2" />
        <InfoCard title="Skewness" :val="asset.skewness" :decimals="4" />
        <InfoCard title="Kurtosis" :val="asset.kurtosis" :decimals="4" />
      </div>
      <div>
        <Histogram :values="[0, 1, 2, 5, 3, 0, 1]" type="percent"/>
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
