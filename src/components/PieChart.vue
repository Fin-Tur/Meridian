<script setup>

import { computed, ref } from 'vue'
import { Pie } from 'vue-chartjs'
import { Chart as ChartJS, ArcElement, Tooltip, Legend } from 'chart.js'
ChartJS.register(ArcElement, Tooltip, Legend)
import{ usePortfolioStore } from '@/stores/counter.js'

const store = usePortfolioStore()

const props = defineProps({
  title: {
    type: String,
    default: ''
  }
})

const pieColors = [
  '#6c5ce7', '#00cec9', '#fd79a8', '#feca57',
  '#a29bfe', '#55efc4', '#fab1a0', '#74b9ff',
  '#dfe6e9', '#81ecec', '#ff7675', '#fdcb6e',
]

const pieData = computed(() => {
  if (!store.assets || store.assets.length === 0) return null
  return {
    labels: store.assets.map(v => v.symbol),
    datasets: [{
      data: store.assets.map(v => store.weights[v.symbol] * store.portfolio_value),
      backgroundColor: pieColors.slice(0, store.assets.length),
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
          return ` ${ctx.label}: $${val.toLocaleString()}`
        }
      }
    }
  }
}
</script>

<template>
            <div class="bg-bg-card border border-border rounded-xl p-6 flex flex-col transition hover:bg-bg-card-hover">
          <div class="text-xs font-semibold uppercase tracking-wider text-text-secondary mb-3">{{title}}</div>
          <div class="flex-1 min-h-72 relative">
            <Pie v-if="pieData" :data="pieData" :options="pieOptions" />
          </div>
        </div>
</template>