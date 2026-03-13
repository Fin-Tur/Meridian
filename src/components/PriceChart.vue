<script setup>
import { computed, ref } from 'vue'
import { Line } from 'vue-chartjs'
import {
  Chart as ChartJS, LineElement, PointElement, CategoryScale,
  LinearScale, Filler, Tooltip
} from 'chart.js'

ChartJS.register(LineElement, PointElement, CategoryScale, LinearScale, Filler, Tooltip)

const props = defineProps({
  values: { type: Array, required: true },
  title:  { type: String, default: '' },
  currency: { type: String, default: '$' }
})

const days = ref(null)   // null = show all

const sym = computed(() => props.currency || '$')

const slicedValues = computed(() => {
  const vals = props.values ?? []
  if (!days.value || days.value >= vals.length) return vals
  return vals.slice(-days.value)
})

const chartData = computed(() => {
  const vals = slicedValues.value
  return {
    labels: vals.map((_, i) => i),
    datasets: [{
      data: vals,
      borderColor: '#6c5ce7',
      backgroundColor: 'rgba(108,92,231,0.08)',
      borderWidth: 1.5,
      pointRadius: 0,
      pointHitRadius: 6,
      tension: 0.3,
      fill: true,
    }]
  }
})

const chartOptions = computed(() => {
  const currency = sym.value
  return {
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
        callbacks: {
          title: ctx => `Day ${ctx[0].label}`,
          label: ctx => `${currency}${ctx.parsed.y.toLocaleString(undefined, { minimumFractionDigits: 2, maximumFractionDigits: 2 })}`
        }
      },
    },
    scales: {
      x: {
        grid: { color: 'rgba(42,45,62,0.5)' },
        ticks: { color: '#8b8fa3', autoSkip: true, maxTicksLimit: 10, font: { size: 11 } },
        title: { display: true, text: 'Days', color: '#8b8fa3', font: { size: 11 } }
      },
      y: {
        grid: { color: 'rgba(42,45,62,0.5)' },
        ticks: {
          color: '#8b8fa3',
          font: { size: 11 },
          callback: v => currency + v.toLocaleString(undefined, { minimumFractionDigits: 2, maximumFractionDigits: 2 })
        }
      }
    }
  }
})
</script>

<template>
  <div class="bg-bg-card border border-border rounded-xl p-6 transition hover:bg-bg-card-hover">
    <div class="flex items-center justify-between mb-3">
      <span v-if="title" class="text-xs font-semibold uppercase tracking-wider text-text-secondary">{{ title }}</span>
      <div class="flex items-center gap-3">
        <div class="flex items-center gap-1.5">
          <label class="text-xs text-text-secondary">Days</label>
          <input
            v-model.number="days"
            type="number"
            min="1"
            :max="values.length"
            :placeholder="values.length"
            class="w-16 bg-transparent border-b border-border px-1 py-0.5 text-text-primary text-xs text-center focus:outline-none focus:border-accent transition-colors [appearance:textfield] [&::-webkit-outer-spin-button]:appearance-none [&::-webkit-inner-spin-button]:appearance-none"
          />
        </div>
        <span class="text-xs text-text-secondary">{{ slicedValues.length }} / {{ values.length }} shown</span>
      </div>
    </div>
    <div class="min-h-72 relative">
      <Line :data="chartData" :options="chartOptions" />
    </div>
  </div>
</template>
