<script setup>
import { ref, onMounted, computed } from 'vue'
import { Pie } from 'vue-chartjs'
import { Chart as ChartJS, ArcElement, Tooltip, Legend } from 'chart.js'
import { fetch_portfolio, fetch_correlation } from '@/services/api.js'
import { usePortfolioStore } from '@/stores/counter.js'
import InfoCard from '@/components/InfoCard.vue'
import SearchBar from '@/components/SearchBar.vue'

ChartJS.register(ArcElement, Tooltip, Legend)

const store = usePortfolioStore()

const portfolio = ref(null)
const correlation = ref(null)
const loading = ref(true)

//Add-asset form
const newSymbol = ref('')
const newQuantity = ref(1)

async function loadData() {
  loading.value = true
  const [p, c] = await Promise.all([fetch_portfolio(), fetch_correlation()])
  portfolio.value = p
  correlation.value = c
  loading.value = false
}

onMounted(loadData)

function addAsset() {
  const sym = newSymbol.value.trim().toUpperCase()
  if (!sym) return
  store.addAsset(sym, newQuantity.value)
  newSymbol.value = ''
  newQuantity.value = 1
  loadData()
}

function removeAsset(symbol) {
  store.removeAsset(symbol)
  loadData()
}

const pieColors = [
  '#6c5ce7', '#00cec9', '#fd79a8', '#feca57',
  '#a29bfe', '#55efc4', '#fab1a0', '#74b9ff',
  '#dfe6e9', '#81ecec', '#ff7675', '#fdcb6e',
]

const pieData = computed(() => {
  if (!portfolio.value?.assets) return null
  return {
    labels: portfolio.value.assets.map(a => a.symbol),
    datasets: [{
      data: portfolio.value.assets.map(a => a.value),
      backgroundColor: pieColors.slice(0, portfolio.value.assets.length),
      borderColor: 'transparent',
      hoverOffset: 8,
    }],
  }
})

const pieOptions = {
  responsive: true,
  maintainAspectRatio: false,
  plugins: {
    legend: {
      position: 'right',
      labels: {
        color: '#8b8fa3',
        padding: 16,
        usePointStyle: true,
        pointStyleWidth: 10,
        font: { size: 13 },
      },
    },
    tooltip: {
      backgroundColor: '#1e2130',
      titleColor: '#e4e6f0',
      bodyColor: '#8b8fa3',
      borderColor: '#2a2d3e',
      borderWidth: 1,
      cornerRadius: 8,
      callbacks: {
        label: ctx => {
          const val = ctx.parsed
          const total = ctx.dataset.data.reduce((a, b) => a + b, 0)
          const pct = ((val / total) * 100).toFixed(1)
          return ` ${ctx.label}: $${val.toLocaleString()} (${pct}%)`
        }
      }
    }
  }
}

function fmtPct(val) {
  if (val == null) return '–'
  return (val >= 0 ? '+' : '') + val.toFixed(2) + '%'
}

function fmtUsd(val) {
  if (val == null) return '–'
  return '$' + val.toLocaleString(undefined, { minimumFractionDigits: 2, maximumFractionDigits: 2 })
}
</script>

<template>
  <div class="max-w-6xl mx-auto px-6 py-8">
    <h1 class="text-2xl font-bold mb-6">My Portfolio</h1>

    <!-- Add asset form -->
    <div class="bg-bg-card border border-border rounded-xl p-6 mb-6 transition hover:bg-bg-card-hover">
      <div class="text-xs font-semibold uppercase tracking-wider text-text-secondary mb-3">Manage Assets</div>
      <div class="flex gap-3 mb-3">
        <SearchBar v-model="newSymbol" :text="'Enter asset symbol (e.g. AAPL)'" @search="addAsset" />
        <input
          v-model.number="newQuantity"
          type="number"
          min="1"
          placeholder="Qty"
          @keydown.enter="addAsset"
          class="w-24 bg-bg-secondary border border-border text-text-primary px-4 py-2.5 rounded-lg text-sm outline-none transition focus:border-accent"
        />
        <button @click="addAsset" class="px-5 py-2.5 bg-accent text-white font-semibold text-sm rounded-lg transition hover:bg-accent-hover cursor-pointer">
          Add Asset
        </button>
      </div>
      <div v-if="store.assets.length" class="flex flex-wrap gap-2">
        <span v-for="a in store.assets" :key="a.symbol" class="inline-flex items-center gap-1.5 px-3 py-1 rounded-full bg-accent-muted text-accent text-xs font-semibold">
          {{ a.symbol }}
          <span class="text-text-secondary font-normal">×{{ a.quantity }}</span>
          <button @click="removeAsset(a.symbol)" class="bg-transparent border-none text-text-secondary cursor-pointer text-sm leading-none transition hover:text-negative">×</button>
        </span>
      </div>
    </div>

    <div v-if="loading" class="flex items-center justify-center min-h-72 text-text-secondary">
      <div class="spinner"></div>
      Loading portfolio…
    </div>

    <template v-else-if="portfolio">
      <!-- Summary row -->
      <div class="grid grid-cols-1 md:grid-cols-3 gap-4 mb-6">
        <div class="bg-bg-card border border-border rounded-xl p-6 transition hover:bg-bg-card-hover">
          <div class="text-xs font-semibold uppercase tracking-wider text-text-secondary mb-2">Total Value</div>
          <div class="text-2xl font-bold">{{ fmtUsd(portfolio.total_value) }}</div>
        </div>
        <div class="bg-bg-card border border-border rounded-xl p-6 transition hover:bg-bg-card-hover">
          <div class="text-xs font-semibold uppercase tracking-wider text-text-secondary mb-2">Assets</div>
          <div class="text-2xl font-bold">{{ portfolio.assets?.length ?? 0 }}</div>
        </div>
        <div class="bg-bg-card border border-border rounded-xl p-6 transition hover:bg-bg-card-hover">
          <div class="text-xs font-semibold uppercase tracking-wider text-text-secondary mb-2">Portfolio Return</div>
          <div class="text-2xl font-bold" :class="portfolio.total_return >= 0 ? 'text-positive' : 'text-negative'">
            {{ fmtPct(portfolio.total_return) }}
          </div>
        </div>
      </div>

      <!-- Pie chart + Table -->
      <div class="grid grid-cols-1 lg:grid-cols-2 gap-6 mb-6">
        <div class="bg-bg-card border border-border rounded-xl p-6 flex flex-col transition hover:bg-bg-card-hover">
          <div class="text-xs font-semibold uppercase tracking-wider text-text-secondary mb-3">Allocation</div>
          <div class="flex-1 min-h-72 relative">
            <Pie v-if="pieData" :data="pieData" :options="pieOptions" />
          </div>
        </div>

        <div class="bg-bg-card border border-border rounded-xl p-6 flex flex-col transition hover:bg-bg-card-hover">
          <div class="text-xs font-semibold uppercase tracking-wider text-text-secondary mb-3">Holdings</div>
          <div class="overflow-x-auto">
            <table class="w-full text-sm">
              <thead>
                <tr>
                  <th class="text-left px-4 py-3 font-semibold text-text-secondary text-xs uppercase tracking-wide border-b border-border">Symbol</th>
                  <th class="text-left px-4 py-3 font-semibold text-text-secondary text-xs uppercase tracking-wide border-b border-border">Value</th>
                  <th class="text-left px-4 py-3 font-semibold text-text-secondary text-xs uppercase tracking-wide border-b border-border">Weight</th>
                  <th class="text-left px-4 py-3 font-semibold text-text-secondary text-xs uppercase tracking-wide border-b border-border">Return</th>
                  <th class="border-b border-border"></th>
                </tr>
              </thead>
              <tbody>
                <tr v-for="asset in portfolio.assets" :key="asset.symbol">
                  <td class="px-4 py-3 border-b border-border font-semibold text-accent">{{ asset.symbol }}</td>
                  <td class="px-4 py-3 border-b border-border">{{ fmtUsd(asset.value) }}</td>
                  <td class="px-4 py-3 border-b border-border">{{ (asset.weight * 100).toFixed(1) }}%</td>
                  <td class="px-4 py-3 border-b border-border">
                    <span class="inline-block px-2.5 py-0.5 rounded-full text-xs font-semibold"
                          :class="asset.return >= 0 ? 'bg-positive/15 text-positive' : 'bg-negative/15 text-negative'">
                      {{ fmtPct(asset.return) }}
                    </span>
                  </td>
                  <td class="px-4 py-3 border-b border-border">
                    <button @click="removeAsset(asset.symbol)" title="Remove"
                            class="bg-transparent border-none text-text-secondary cursor-pointer p-1 rounded-lg transition hover:text-negative hover:bg-negative/10">
                      ✕
                    </button>
                  </td>
                </tr>
              </tbody>
            </table>
          </div>
        </div>
      </div>

      <!-- Correlation matrix -->
      <div v-if="correlation" class="bg-bg-card border border-border rounded-xl p-6 transition hover:bg-bg-card-hover">
        <div class="text-xs font-semibold uppercase tracking-wider text-text-secondary mb-3">Correlation Matrix</div>
        <div class="overflow-x-auto">
          <table class="w-full text-sm">
            <thead>
              <tr>
                <th class="px-4 py-3 border-b border-border"></th>
                <th v-for="sym in correlation.symbols" :key="sym" class="px-4 py-3 text-center font-semibold text-text-secondary text-xs uppercase tracking-wide border-b border-border">{{ sym }}</th>
              </tr>
            </thead>
            <tbody>
              <tr v-for="(row, i) in correlation.matrix" :key="i">
                <td class="px-4 py-3 border-b border-border font-semibold text-accent">{{ correlation.symbols[i] }}</td>
                <td v-for="(val, j) in row" :key="j"
                    class="px-4 py-3 border-b border-border text-center text-xs tabular-nums"
                    :style="{ background: corrColor(val) }">
                  {{ val.toFixed(2) }}
                </td>
              </tr>
            </tbody>
          </table>
        </div>
      </div>
    </template>

    <div v-else class="flex items-center justify-center min-h-72 text-text-secondary">No portfolio data available.</div>
  </div>
</template>

<script>
export default {
  methods: {
    corrColor(val) {
      if (val >= 0.8) return 'rgba(0,206,201,0.25)'
      if (val >= 0.4) return 'rgba(0,206,201,0.12)'
      if (val <= -0.4) return 'rgba(255,107,107,0.12)'
      if (val <= -0.8) return 'rgba(255,107,107,0.25)'
      return 'transparent'
    }
  }
}
</script>
