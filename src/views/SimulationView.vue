<script setup>
import { ref, onMounted, computed } from 'vue'
import { Bar } from 'vue-chartjs'
import {
  Chart as ChartJS, BarElement, CategoryScale, LinearScale,
  Tooltip, Legend
} from 'chart.js'
import { fetch_simulation } from '@/services/api.js'
import InfoCard from '@/components/InfoCard.vue'
import Histogram from '@/components/Histogram.vue'
import { usePortfolioStore } from '@/stores/counter.js'

ChartJS.register(BarElement, CategoryScale, LinearScale, Tooltip, Legend)

const simulation = ref(null)
const loading = ref(true)
const store = usePortfolioStore()

onMounted(async () => {
  simulation.value = await fetch_simulation()
  loading.value = false
})

const histLabels = computed(() => {
  if (!simulation.value) return []
  const labels = []
  for (let i = 0; i < 50; i++) {
    const binAmount = ((simulation.value.min + i * simulation.value.bin_width)).toFixed(0)
    const pct = store.pctOfPortfolio(binAmount)
    labels.push((pct-100).toFixed(2))
  }
  return labels
})

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
        <InfoCard title="Average Return" :val="simulation.avg_return" :type="'beneficial-currency'" :decimals="2" />
        <InfoCard title="Min" :val="simulation.min" :type="'beneficial-percentile'" :decimals="2" />
        <InfoCard title="Max" :val="simulation.max" :type="'beneficial-percentile'" :decimals="2" />
      </div>

      <div class="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-4 gap-4 mb-6">
        <InfoCard title="Var 95%" :val="-simulation.var95" :type="'currency-beneficial'" :decimals="2" />
        <InfoCard title="Var 99%" :val="-simulation.var99" :type="'currency-beneficial'" :decimals="2" />
        <InfoCard title="CVaR 95%" :val="-simulation.cvar95" :type="'currency-beneficial'" :decimals="2" />
        <InfoCard title="CVaR 99%" :val="-simulation.cvar99" :type="'currency-beneficial'" :decimals="2" />
      </div>

      <!-- Histogram -->
      <Histogram :values="simulation.bins" :labels="histLabels" type="distribution-percent" title="Return Distribution" />
    </template>

    <div v-else class="flex items-center justify-center min-h-72 text-text-secondary">No simulation data available.</div>
  </div>
</template>
