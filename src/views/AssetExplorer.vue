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
import PriceChart from '@/components/PriceChart.vue'

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
        <InfoCard title="YTD Returns" :val="asset.ytd_return" :type="'beneficial-percentile'" :decimals="2" />
        <InfoCard title="Skewness" :val="asset.skewness" :decimals="4" />
        <InfoCard title="Kurtosis" :val="asset.kurtosis" :decimals="4" />
      </div>
      <div>
        <PriceChart :values="asset.adj_closes" :currency="asset.currency === 'USD' ? '$' : asset.currency" title="Price History" />
      </div>    
    </template>

    <div v-else-if="searched && !loading" class="flex items-center justify-center min-h-72 text-text-secondary">
      No data found for this symbol.
    </div>
  </div>
</template>
