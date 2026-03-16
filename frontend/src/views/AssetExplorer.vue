<script setup>
import { ref, onMounted } from 'vue'
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
import AssetCard from '@/components/AssetCard.vue'

ChartJS.register(BarElement, CategoryScale, LinearScale, Tooltip, Legend)

const symbol = ref('')
const asset = ref(null)
const loading = ref(false)
const searched = ref(false)

const featuredSymbols = ['AAPL', 'MSFT', 'GOOGL', 'AMZN', 'TSLA', 'NVDA', 'VGK', 'SPY', 'GLD', 'IWDA.AS', 'EIMI.L']
const featuredAssets = ref([])
const featuredLoading = ref(false)

async function search() {
  const sym = symbol.value.trim().toUpperCase()
  if (!sym) return
  loading.value = true
  searched.value = true
  asset.value = await fetch_asset(sym)
  loading.value = false
}

function selectFeatured(sym) {
  symbol.value = sym
  search()
}

onMounted(async () => {
  featuredLoading.value = true
  const results = await Promise.all(
    featuredSymbols.map(s => fetch_asset(s).catch(() => null))
  )
  featuredAssets.value = results.filter(Boolean)
  featuredLoading.value = false
})
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

    <div v-if="!searched && !loading" class="mb-8">
      <h2 class="text-lg font-semibold text-text-secondary mb-4">Popular Assets</h2>
      <div v-if="featuredLoading" class="text-text-secondary text-sm">Loading suggestions…</div>
      <div v-else class="grid grid-cols-2 sm:grid-cols-3 lg:grid-cols-6 gap-4">
        <AssetCard
          v-for="a in featuredAssets" :key="a.symbol"
          :symbol="a.symbol"
          :price="a.adj_closes?.[a.adj_closes.length - 1]"
          :currency="a.currency"
          :ytdReturn="a.ytd_return"
          @select="selectFeatured"
        />
      </div>
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
        <InfoCard title="Volatility" :val="asset.volatility" :type="'percentile'" :decimals="2" :tooltip="'Volatility displays the degree of variation of the asset\'s returns.'"/>
        <InfoCard title="Avg Log Return" :val="asset.avg_log_return" :type="'beneficial'" :decimals="4" :tooltip="'Average log return displays the average logarithmic return of the asset.'"/>
        <InfoCard title="Sharpe Ratio" :val="asset.sharpe_ratio" :decimals="3" :tooltip="'The Sharpe Ratio measures the risk-adjusted return of the asset. A higher Sharpe Ratio indicates better risk-adjusted performance.'"/>
        <InfoCard title="Max Drawdown" :val="-asset.max_drawdown" :type="'beneficial-percentile'" :decimals="2" :tooltip="'Max Drawdown displays the maximum observed loss from a peak to a trough of the asset.'"/>
      </div>

      <div class="grid grid-cols-1 sm:grid-cols-3 gap-4 mb-6">
        <InfoCard title="YTD Returns" :val="asset.ytd_return/100" :type="'beneficial-percentile'" :decimals="2" :tooltip="'Year-to-date returns display the performance of the asset since the beginning of the year.'"/>
        <InfoCard title="Skewness" :val="asset.skewness" :decimals="4" :tooltip="'Skewness displays how symetric the daily log distribution is. Positive Skewness means the median is higher then the average.'"/>
        <InfoCard title="Kurtosis" :val="asset.kurtosis" :decimals="4" :tooltip="'Kurtosis displays how much the daily log return distribution differs from normal distribution'"/>
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
