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
  <div class="page">
    <h1 class="page-title">Asset Explorer</h1>

    <!-- Search -->
    <div class="search-bar">
      <input
        v-model="symbol"
        placeholder="Enter ticker symbol (e.g. AAPL)"
        @keydown.enter="search"
      />
      <button class="btn btn-primary" @click="search" :disabled="loading">
        {{ loading ? 'Loading…' : 'Search' }}
      </button>
    </div>

    <div v-if="loading" class="loading">
      <div class="spinner"></div>
      Fetching asset data…
    </div>

    <template v-else-if="asset">
      <h2 class="asset-symbol">{{ asset.symbol }} <span class="currency-tag">{{ asset.currency }}</span></h2>

      <!-- Stats grid -->
      <div class="grid-4 stat-row">
        <div class="card">
          <div class="card-title">Volatility</div>
          <div class="stat-value">{{ (asset.volatility * 100).toFixed(2) }}%</div>
        </div>
        <div class="card">
          <div class="card-title">Avg Log Return</div>
          <div class="stat-value" :class="asset.avg_log_return >= 0 ? 'positive' : 'negative'">
            {{ fmtPct(asset.avg_log_return) }}
          </div>
        </div>
        <div class="card">
          <div class="card-title">Sharpe Ratio</div>
          <div class="stat-value">{{ asset.sharpe_ratio?.toFixed(3) ?? '–' }}</div>
        </div>
        <div class="card">
          <div class="card-title">Max Drawdown</div>
          <div class="stat-value negative">{{ (asset.max_drawdown * 100).toFixed(2) }}%</div>
        </div>
      </div>

      <div class="grid-3 stat-row-2">
        <div class="card">
          <div class="card-title">YTD Return</div>
          <div class="stat-value" :class="asset.ytd_returns >= 0 ? 'positive' : 'negative'">
            {{ (asset.ytd_returns * 100).toFixed(2) }}%
          </div>
        </div>
        <div class="card">
          <div class="card-title">Skewness</div>
          <div class="stat-value">{{ asset.skewness?.toFixed(4) ?? '–' }}</div>
        </div>
        <div class="card">
          <div class="card-title">Kurtosis</div>
          <div class="stat-value">{{ asset.kurtosis?.toFixed(4) ?? '–' }}</div>
        </div>
      </div>

      <!-- Histogram of daily returns -->
      <div v-if="histData" class="card hist-card">
        <div class="card-title">Daily Return Distribution</div>
        <div class="hist-wrapper">
          <Bar :data="histData" :options="histOptions" />
        </div>
      </div>
    </template>

    <div v-else-if="searched && !loading" class="loading">
      No data found for this symbol.
    </div>
  </div>
</template>

<style scoped>
.search-bar {
  display: flex;
  gap: 0.75rem;
  margin-bottom: 2rem;
}

.search-bar input {
  flex: 1;
  max-width: 400px;
}

.asset-symbol {
  font-size: 1.5rem;
  font-weight: 700;
  margin-bottom: 1.25rem;
  display: flex;
  align-items: center;
  gap: 0.75rem;
}

.currency-tag {
  font-size: 0.8rem;
  font-weight: 600;
  padding: 0.2rem 0.6rem;
  border-radius: 999px;
  background: var(--accent-muted);
  color: var(--accent);
}

.stat-row {
  margin-bottom: 1rem;
}

.stat-row-2 {
  margin-bottom: 1.5rem;
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
