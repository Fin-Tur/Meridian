<script setup>
import { ref, onMounted, computed } from 'vue'
import { Pie } from 'vue-chartjs'
import { Chart as ChartJS, ArcElement, Tooltip, Legend } from 'chart.js'
import { fetch_portfolio, fetch_correlation } from '@/services/api.js'
import { usePortfolioStore } from '@/stores/counter.js'
import InfoCard from '@/components/InfoCard.vue'
import SearchBar from '@/components/SearchBar.vue'
import PieChart from '@/components/PieChart.vue'
import DataTable from '@/components/DataTable.vue'

ChartJS.register(ArcElement, Tooltip, Legend)

const store = usePortfolioStore()

const correlation = ref(null)
const loading = ref(true)

//Add-asset form
const newSymbol = ref('')
const newQuantity = ref(1)

async function loadData() {
  loading.value = true
  const p = await fetch_portfolio()
  const c = await fetch_correlation()
  store.setPortfolioValue(p.portfolio_value)
  for(const weight of p.weights){
  store.setWeight(weight[0], weight[1])
  correlation.value = c
  loading.value = false
  }
  
  
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

function fmtPct(val) {
  if (val == null) return '–'
  return (val >= 0 ? '+' : '') + val.toFixed(2) + '%'
}

function fmtUsd(val) {
  if (val == null) return '–'
  return '$' + val.toLocaleString(undefined, { minimumFractionDigits: 2, maximumFractionDigits: 2 })
}

/* ── Holdings table config ── */
const holdingsColumns = [
  { key: 'symbol', label: 'Symbol', cellClass: 'font-semibold text-accent' },
  { key: 'value',  label: 'Value' },
  { key: 'weight', label: 'Weight' },
  { key: 'return', label: 'Return' },
]

/* ── Correlation table config ── */
const corrColumns = computed(() => {
  if (!correlation.value) return []
  return [
    { key: '_label', label: '', cellClass: 'font-semibold text-accent' },
    ...correlation.value.symbols.map(sym => ({
      key: sym,
      label: sym,
      align: 'center',
      cellClass: 'text-xs tabular-nums',
      cellStyle: (val) => ({ background: corrColor(val) }),
    })),
  ]
})

const corrRows = computed(() => {
  if (!correlation.value) return []
  return correlation.value.matrix.map((row, i) => {
    const obj = { _label: correlation.value.symbols[i] }
    correlation.value.symbols.forEach((sym, j) => { obj[sym] = row[j] })
    return obj
  })
})

function corrColor(val) {
  if (val >= 0.8) return 'rgba(0,206,201,0.25)'
  if (val >= 0.4) return 'rgba(0,206,201,0.12)'
  if (val <= -0.4) return 'rgba(255,107,107,0.12)'
  if (val <= -0.8) return 'rgba(255,107,107,0.25)'
  return 'transparent'
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

    <template v-else-if="store.assets.length">
      
      <!-- Summary row -->
      <div class="grid grid-cols-1 md:grid-cols-3 gap-4 mb-6">
        <InfoCard title="Total Value" :val="store.portfolio_value" :decimals="2" type="currency"/>
        <InfoCard title="Assets" :val="store.assets.length" type="number"/>
      </div>

      <!-- Pie chart + Table -->
      <div class="grid grid-cols-1 lg:grid-cols-2 gap-6 mb-6">
        <PieChart :title="'Allocation'"/>
        
        <DataTable title="Holdings" :columns="holdingsColumns" :rows="store.assets">
          <template #cell-value="{ row }">
            {{ fmtUsd(store.portfolio_value * store.weights[row.symbol]) }}
          </template>
          <template #cell-weight="{ row }">
            {{ (store.weights[row.symbol] * 100).toFixed(1) }}%
          </template>
          <template #cell-return="{ row }">
            <span class="inline-block px-2.5 py-0.5 rounded-full text-xs font-semibold"
                  :class="row.return >= 0 ? 'bg-positive/15 text-positive' : 'bg-negative/15 text-negative'">
              {{ fmtPct(row.return) }}
            </span>
          </template>
        </DataTable>
      </div>

      <!-- Correlation matrix -->
      <DataTable
        v-if="correlation"
        title="Correlation Matrix"
        :columns="corrColumns"
        :rows="corrRows"
      >
        <template #cell="{ value, col }">
          {{ col.key !== '_label' && typeof value === 'number' ? value.toFixed(2) : value }}
        </template>
      </DataTable>
    </template>

    <div v-else class="flex items-center justify-center min-h-72 text-text-secondary">No portfolio data available.</div>
  </div>
</template>


