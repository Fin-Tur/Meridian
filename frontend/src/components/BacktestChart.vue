<script setup>
import { computed } from 'vue'
import { Line } from 'vue-chartjs'
import {
  Chart as ChartJS, LineElement, PointElement, CategoryScale,
  LinearScale, Filler, Tooltip, Legend
} from 'chart.js'

ChartJS.register(LineElement, PointElement, CategoryScale, LinearScale, Filler, Tooltip, Legend)

const props = defineProps({
  actualValues:     { type: Array, required: true },
  simulatedValues:  { type: Array, required: true },
  exceedances95:    { type: Array, required: true },
  exceedances99:    { type: Array, required: true },
})

// Build exceedance marker datasets: show actual value at breach periods, null otherwise
function buildExceedanceData(flags, values) {
  return flags.map((flag, i) => flag ? values[i] : null)
}

const allValues = computed(() => [
  ...props.actualValues,
  ...props.simulatedValues,
].filter(v => v != null))

const yMin = computed(() => {
  const min = Math.min(...allValues.value)
  const max = Math.max(...allValues.value)
  const padding = (max - min) * 0.05
  return Math.floor(min - padding)
})

const yMax = computed(() => {
  const min = Math.min(...allValues.value)
  const max = Math.max(...allValues.value)
  const padding = (max - min) * 0.05
  return Math.ceil(max + padding)
})

const chartData = computed(() => {
  const len = props.actualValues.length
  const labels = Array.from({ length: len }, (_, i) => i + 1)

  const exc95 = buildExceedanceData(props.exceedances95, props.actualValues)
  const exc99 = buildExceedanceData(props.exceedances99, props.actualValues)

  return {
    labels,
    datasets: [
      {
        label: 'Actual Portfolio Value',
        data: props.actualValues,
        borderColor: '#6c5ce7',
        backgroundColor: 'rgba(108,92,231,0.06)',
        borderWidth: 2,
        pointRadius: 0,
        pointHitRadius: 6,
        tension: 0.3,
        fill: false,
      },
      {
        label: 'Simulated Portfolio Value',
        data: props.simulatedValues,
        borderColor: '#00cec9',
        backgroundColor: 'rgba(0,206,201,0.06)',
        borderWidth: 2,
        pointRadius: 0,
        pointHitRadius: 6,
        tension: 0.3,
        fill: false,
      },
      {
        label: 'VaR 95% Breach',
        data: exc95,
        borderColor: 'transparent',
        backgroundColor: '#fdcb6e',
        borderWidth: 0,
        pointRadius: 6,
        pointHoverRadius: 8,
        pointStyle: 'triangle',
        pointHitRadius: 10,
        showLine: false,
        spanGaps: false,
      },
      {
        label: 'VaR 99% Breach',
        data: exc99,
        borderColor: 'transparent',
        backgroundColor: '#e17055',
        borderWidth: 0,
        pointRadius: 7,
        pointHoverRadius: 9,
        pointStyle: 'rectRot',
        pointHitRadius: 10,
        showLine: false,
        spanGaps: false,
      },
    ]
  }
})

const chartOptions = computed(() => ({
  responsive: true,
  maintainAspectRatio: false,
  interaction: { mode: 'index', intersect: false },
  plugins: {
    legend: {
      display: true,
      position: 'top',
      labels: {
        color: '#8b8fa3',
        usePointStyle: true,
        font: { size: 11 },
        padding: 16,
      },
    },
    tooltip: {
      backgroundColor: '#1e2130',
      titleColor: '#e4e6f0',
      bodyColor: '#8b8fa3',
      borderColor: '#2a2d3e',
      borderWidth: 1,
      cornerRadius: 8,
      filter: ctx => ctx.parsed.y != null,
      callbacks: {
        title: ctx => `Period ${ctx[0].label}`,
        label: ctx => `${ctx.dataset.label}: $${ctx.parsed.y.toLocaleString(undefined, { minimumFractionDigits: 2, maximumFractionDigits: 2 })}`
      }
    },
  },
  scales: {
    x: {
      grid: { color: 'rgba(42,45,62,0.5)' },
      ticks: { color: '#8b8fa3', autoSkip: true, maxTicksLimit: 12, font: { size: 11 } },
      title: { display: true, text: 'Testing Period', color: '#8b8fa3', font: { size: 11 } }
    },
    y: {
      min: yMin.value,
      max: yMax.value,
      grid: { color: 'rgba(42,45,62,0.5)' },
      ticks: {
        color: '#8b8fa3',
        font: { size: 11 },
        callback: v => '$' + v.toLocaleString(undefined, { minimumFractionDigits: 0, maximumFractionDigits: 0 })
      },
      title: { display: true, text: 'Portfolio Value ($)', color: '#8b8fa3', font: { size: 11 } }
    }
  }
}))
</script>

<template>
  <div class="bg-bg-card border border-border rounded-xl p-6 transition hover:bg-bg-card-hover">
    <span class="text-xs font-semibold uppercase tracking-wider text-text-secondary mb-3 block">
      Backtest — Portfolio Value vs. VaR Exceedances
    </span>
    <div class="min-h-80 relative">
      <Line :data="chartData" :options="chartOptions" />
    </div>
  </div>
</template>
