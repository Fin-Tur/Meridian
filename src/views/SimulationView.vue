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

ChartJS.register(BarElement, CategoryScale, LinearScale, Tooltip, Legend)

const simulation = ref(null)
const loading = ref(true)

onMounted(async () => {
  simulation.value = await fetch_simulation()
  loading.value = false
})

const histLabels = computed(() => {
  if (!simulation.value) return []
  const { bin_size, bins } = simulation.value
  const labels = []
  for (let i = 0; i < bins.length; i++) {
    
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
        <InfoCard title="Mean Return" :val="simulation.avg_return" :type="'beneficial-percentile'" :decimals="2" />
        <InfoCard title="Min" :val="simulation.min" :type="'beneficial-percentile'" :decimals="2" />
        <InfoCard title="Max" :val="simulation.max" :type="'beneficial-percentile'" :decimals="2" />
      </div>

      <div class="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-4 gap-4 mb-6">
        <InfoCard title="Var 95%" :val="simulation.var95" :type="'beneficial-percentile'" :decimals="2" />
        <InfoCard title="Var 99%" :val="simulation.var99" :type="'beneficial-percentile'" :decimals="2" />
        <InfoCard title="CVaR 95%" :val="simulation.cvar95" :type="'beneficial-percentile'" :decimals="2" />
        <InfoCard title="CVaR 99%" :val="simulation.cvar99" :type="'beneficial-percentile'" :decimals="2" />
      </div>

      <!-- Histogram -->
      <Histogram :values="simulation.bins" :labels="simulation.labels" type="percent" title="Return Distribution" />
    </template>

    <div v-else class="flex items-center justify-center min-h-72 text-text-secondary">No simulation data available.</div>
  </div>
</template>
