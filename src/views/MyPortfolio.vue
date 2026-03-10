<script setup>
import { ref, onMounted, computed } from 'vue'
import { Pie } from 'vue-chartjs'
import { Chart as ChartJS, ArcElement, Tooltip, Legend } from 'chart.js'
import { fetch_portfolio, fetch_correlation } from '@/services/api.js'
import { usePortfolioStore } from '@/stores/counter.js'

ChartJS.register(ArcElement, Tooltip, Legend)

const store = usePortfolioStore()

const portfolio = ref(null)
const correlation = ref(null)
const loading = ref(true)

// Add-asset form
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
  <div class="page">
    <h1 class="page-title">My Portfolio</h1>

    <!-- Add asset form -->
    <div class="card add-asset-card">
      <div class="card-title">Manage Assets</div>
      <div class="add-asset-form">
        <input
          v-model="newSymbol"
          placeholder="Symbol (e.g. AAPL)"
          @keydown.enter="addAsset"
        />
        <input
          v-model.number="newQuantity"
          type="number"
          min="1"
          placeholder="Qty"
          class="qty-input"
          @keydown.enter="addAsset"
        />
        <button class="btn btn-primary" @click="addAsset">Add Asset</button>
      </div>
      <div v-if="store.assets.length" class="asset-tags">
        <span v-for="a in store.assets" :key="a.symbol" class="asset-tag">
          {{ a.symbol }} <span class="tag-qty">×{{ a.quantity }}</span>
          <button class="tag-remove" @click="removeAsset(a.symbol)">×</button>
        </span>
      </div>
    </div>

    <div v-if="loading" class="loading">
      <div class="spinner"></div>
      Loading portfolio…
    </div>

    <template v-else-if="portfolio">
      <!-- Summary row -->
      <div class="grid-3 summary-row">
        <div class="card">
          <div class="card-title">Total Value</div>
          <div class="stat-value">{{ fmtUsd(portfolio.total_value) }}</div>
        </div>
        <div class="card">
          <div class="card-title">Assets</div>
          <div class="stat-value">{{ portfolio.assets?.length ?? 0 }}</div>
        </div>
        <div class="card">
          <div class="card-title">Portfolio Return</div>
          <div class="stat-value" :class="portfolio.total_return >= 0 ? 'positive' : 'negative'">
            {{ fmtPct(portfolio.total_return) }}
          </div>
        </div>
      </div>

      <!-- Pie chart + Table -->
      <div class="content-grid">
        <div class="card pie-card">
          <div class="card-title">Allocation</div>
          <div class="pie-wrapper">
            <Pie v-if="pieData" :data="pieData" :options="pieOptions" />
          </div>
        </div>

        <div class="card table-card">
          <div class="card-title">Holdings</div>
          <div class="table-scroll">
            <table class="data-table">
              <thead>
                <tr>
                  <th>Symbol</th>
                  <th>Value</th>
                  <th>Weight</th>
                  <th>Return</th>
                  <th></th>
                </tr>
              </thead>
              <tbody>
                <tr v-for="asset in portfolio.assets" :key="asset.symbol">
                  <td class="symbol-cell">{{ asset.symbol }}</td>
                  <td>{{ fmtUsd(asset.value) }}</td>
                  <td>{{ (asset.weight * 100).toFixed(1) }}%</td>
                  <td>
                    <span :class="asset.return >= 0 ? 'badge badge-green' : 'badge badge-red'">
                      {{ fmtPct(asset.return) }}
                    </span>
                  </td>
                  <td>
                    <button class="btn-remove" @click="removeAsset(asset.symbol)" title="Remove">✕</button>
                  </td>
                </tr>
              </tbody>
            </table>
          </div>
        </div>
      </div>

      <!-- Correlation matrix -->
      <div v-if="correlation" class="card correlation-card">
        <div class="card-title">Correlation Matrix</div>
        <div class="table-scroll">
          <table class="data-table corr-table">
            <thead>
              <tr>
                <th></th>
                <th v-for="sym in correlation.symbols" :key="sym">{{ sym }}</th>
              </tr>
            </thead>
            <tbody>
              <tr v-for="(row, i) in correlation.matrix" :key="i">
                <td class="symbol-cell">{{ correlation.symbols[i] }}</td>
                <td v-for="(val, j) in row" :key="j"
                    :style="{ background: corrColor(val) }">
                  {{ val.toFixed(2) }}
                </td>
              </tr>
            </tbody>
          </table>
        </div>
      </div>
    </template>

    <div v-else class="loading">No portfolio data available.</div>
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

<style scoped>
.summary-row {
  margin-bottom: 1.5rem;
}

.content-grid {
  display: grid;
  grid-template-columns: 1fr 1fr;
  gap: 1.5rem;
  margin-bottom: 1.5rem;
}

@media (max-width: 900px) {
  .content-grid {
    grid-template-columns: 1fr;
  }
}

.pie-card {
  display: flex;
  flex-direction: column;
}

.pie-wrapper {
  flex: 1;
  min-height: 280px;
  position: relative;
}

.table-card {
  display: flex;
  flex-direction: column;
}

.table-scroll {
  overflow-x: auto;
}

.symbol-cell {
  font-weight: 600;
  color: var(--accent);
}

.correlation-card {
  margin-top: 0;
}

.corr-table td {
  text-align: center;
  font-size: 0.85rem;
  font-variant-numeric: tabular-nums;
}

.corr-table th {
  text-align: center;
}

.add-asset-card {
  margin-bottom: 1.5rem;
}

.add-asset-form {
  display: flex;
  gap: 0.75rem;
  margin-bottom: 0.75rem;
}

.add-asset-form input {
  max-width: 260px;
}

.qty-input {
  max-width: 100px !important;
}

.asset-tags {
  display: flex;
  flex-wrap: wrap;
  gap: 0.5rem;
}

.asset-tag {
  display: inline-flex;
  align-items: center;
  gap: 0.35rem;
  padding: 0.3rem 0.65rem;
  border-radius: 999px;
  background: var(--accent-muted);
  color: var(--accent);
  font-size: 0.82rem;
  font-weight: 600;
}

.tag-qty {
  color: var(--text-secondary);
  font-weight: 400;
}

.tag-remove {
  background: none;
  border: none;
  color: var(--text-secondary);
  cursor: pointer;
  font-size: 0.9rem;
  padding: 0 0.15rem;
  line-height: 1;
  transition: color var(--transition);
}

.tag-remove:hover {
  color: var(--red);
}

.btn-remove {
  background: none;
  border: none;
  color: var(--text-secondary);
  cursor: pointer;
  font-size: 0.95rem;
  padding: 0.2rem 0.4rem;
  border-radius: var(--radius-sm);
  transition: all var(--transition);
}

.btn-remove:hover {
  color: var(--red);
  background: rgba(255, 107, 107, 0.12);
}
</style>
