<script setup>
import { computed } from 'vue'
import { Bar } from 'vue-chartjs'
import {
  Chart as ChartJS, BarElement, CategoryScale, LinearScale,
  Tooltip, Legend
} from 'chart.js'

ChartJS.register(BarElement, CategoryScale, LinearScale, Tooltip, Legend)

const props = defineProps({
  /**
   * Array of numeric values — each value becomes one bar.
   * The index determines the position, the value determines the height.
   */
  values: {
    type: Array,
    required: true
  },
  /**
   * Labels for each bar. Must match the length of `values`.
   * If omitted, bar indices are used.
   */
  labels: {
    type: Array,
    default: null
  },
  /**
   * 'percent'   → labels are formatted as percentages (e.g. "2.50%"),
   *              bars < 0 are red, bars >= 0 are teal
   * 'absolute'  → labels are shown as-is, all bars are teal
   */
  type: {
    type: String,
    default: 'absolute',
    validator: v => ['percent', 'absolute'].includes(v)
  },
  title: {
    type: String,
    default: ''
  }
})

const chartData = computed(() => {
  const vals = props.values ?? []

  const resolvedLabels = props.labels
    ? props.labels
    : vals.map((_, i) => String(i))

  return {
    labels: resolvedLabels,
    datasets: [{
      label: 'Frequency',
      data: vals,
      backgroundColor: vals.map(v =>
        props.type === 'percent' && v < 0
          ? 'rgba(255,107,107,0.7)'
          : 'rgba(0,206,201,0.7)'
      ),
      borderColor: vals.map(v =>
        props.type === 'percent' && v < 0
          ? '#ff6b6b'
          : '#00cec9'
      ),
      borderWidth: 1,
      borderRadius: 2,
      barPercentage: 1,
      categoryPercentage: 1,
    }]
  }
})

const chartOptions = {
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
    },
  },
  scales: {
    x: {
      grid: { color: 'rgba(42,45,62,0.5)' },
      ticks: { color: '#8b8fa3', maxRotation: 45, autoSkip: true, maxTicksLimit: 15, font: { size: 11 } },
    },
    y: {
      grid: { color: 'rgba(42,45,62,0.5)' },
      ticks: { color: '#8b8fa3', font: { size: 11 } },
    },
  },
}
</script>

<template>
  <div class="bg-bg-card border border-border rounded-xl p-6 flex flex-col transition hover:bg-bg-card-hover">
    <div v-if="title" class="text-xs font-semibold uppercase tracking-wider text-text-secondary mb-3">
      {{ title }}
    </div>
    <div class="min-h-90 relative">
      <Bar :data="chartData" :options="chartOptions" />
    </div>
  </div>
</template>
