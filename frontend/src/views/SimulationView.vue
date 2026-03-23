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
import UnitToggle from '@/components/UnitToggle.vue'
import { usePortfolioStore } from '@/stores/portfolio.js'
import { useSimConfigStore } from '@/stores/sim_config.js'
import BackArrow from '@/components/BackArrow.vue'
import TooltipComp from '@/components/TooltipComp.vue'

ChartJS.register(BarElement, CategoryScale, LinearScale, Tooltip, Legend)

const simulation = ref(null)
const loading = ref(false)
const portfolio = usePortfolioStore()
const simConfig = useSimConfigStore()

const display_unit = ref('$')

async function start_simulation(){
  loading.value = true
  simulation.value = await fetch_simulation()
  loading.value = false
}

const histLabels = computed(() => {
  if (!simulation.value) return []
  const labels = []
  for (let i = 0; i < 50; i++) {
    const binAmount = ((simulation.value.min + i * simulation.value.bin_width)).toFixed(0)
    const pct = portfolio.pctOfPortfolio(binAmount)
    labels.push((pct-100).toFixed(2))
  }
  return labels
})

function openSettings(){
  simulation.value = null
  loading.value = false
}

</script>
<template>
  <div class="max-w-6xl mx-auto px-6 py-8">
      
    <div class="flex items-center justify-between mb-6">
      <div class="relative flex items-center">
        <BackArrow v-if="simulation" @back="openSettings" class="absolute right-full mr-6" />
        <h1 class="text-2xl font-bold">Monte Carlo Simulation</h1>
      </div>
      <UnitToggle v-if="simulation" v-model="display_unit" />
    </div>

    <div v-if="loading" class="flex items-center justify-center min-h-72 text-text-secondary">
      <div class="spinner"></div>
      Running simulation…
    </div>

    <template v-else-if="simulation">
      <!-- Stats -->
      <div class="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-4 gap-4 mb-4">
        <InfoCard title="Starting Portfolio Value" :val="portfolio.portfolio_value" :type="'currency'" :decimals="2" />
        <InfoCard v-if="display_unit==='$'" title="Median Return" :val="simulation.med_return-portfolio.portfolio_value" :type="'beneficial-currency'" :decimals="2" :tooltip="'Median Return over all simulation values.'"/>
        <InfoCard v-else title="Median Return" :val="(portfolio.pctOfPortfolio(simulation.med_return)/100)-1" :type="'percentile-beneficial'" :decimals="2" :tooltip="'Median Return over all simulation values.'"/>
        <InfoCard v-if="display_unit==='$'" title="Average Return" :val="simulation.avg_return-portfolio.portfolio_value" :type="'beneficial-currency'" :decimals="2" :tooltip="'Average Return over all simulation values. This may be distored by extreme outliers in the simulation.'"/>
        <InfoCard v-else title="Average Return" :val="((portfolio.pctOfPortfolio(simulation.avg_return)-100)/100)" :type="'beneficial-percentile'" :decimals="2" :tooltip="'Average Return over all simulation values. This may be distored by extreme outliers in the simulation.'"/>
        <InfoCard v-if="display_unit==='$'" title="Max Loss" :val="simulation.min-portfolio.portfolio_value" :type="'beneficial-currency'" :decimals="2" :tooltip="'Maximum Loss over all simulation values.'"/>
        <InfoCard v-else title="Max Loss" :val="(portfolio.pctOfPortfolio(simulation.min)/100)-1" :type="'percentile-beneficial'" :decimals="2" :tooltip="'Maximum Loss over all simulation values.'"/>
      </div>

      <div class="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-4 gap-4 mb-6">
        <InfoCard v-if="display_unit==='$'" title="Var 95%" :val="-simulation.var95" :type="'currency-beneficial'" :decimals="2" :tooltip="'Value at Risk (95%) over all simulation values.'"/>
        <InfoCard v-else title="Var 95%" :val="-portfolio.pctOfPortfolio(simulation.var95)/100" :type="'percentile-beneficial'" :decimals="2" :tooltip="'Value at Risk (95%) over all simulation values.'"/>
        <InfoCard v-if="display_unit==='$'" title="Var 99%" :val="-simulation.var99" :type="'currency-beneficial'" :decimals="2" :tooltip="'Value at Risk (99%) over all simulation values.'"/>
        <InfoCard v-else title="Var 99%" :val="-portfolio.pctOfPortfolio(simulation.var99)/100" :type="'percentile-beneficial'" :decimals="2" :tooltip="'Value at Risk (99%) over all simulation values.'"/>
        <InfoCard v-if="display_unit==='$'" title="CVaR 95%" :val="-simulation.cvar95" :type="'currency-beneficial'" :decimals="2" :tooltip="'Conditional Value at Risk (95%) over all simulation values.'"/>
        <InfoCard v-else title="CVaR 95%" :val="-portfolio.pctOfPortfolio(simulation.cvar95)/100" :type="'percentile-beneficial'" :decimals="2" :tooltip="'Conditional Value at Risk (95%) over all simulation values.'"/>
        <InfoCard v-if="display_unit==='$'" title="CVaR 99%" :val="-simulation.cvar99" :type="'currency-beneficial'" :decimals="2" :tooltip="'Conditional Value at Risk (99%) over all simulation values.'"/>
        <InfoCard v-else title="CVaR 99%" :val="-portfolio.pctOfPortfolio(simulation.cvar99)/100" :type="'percentile-beneficial'" :decimals="2" :tooltip="'Conditional Value at Risk (99%) over all simulation values.'"/>
      </div>

      <!-- Histogram -->
      <Histogram :values="simulation.bins" :labels="histLabels" type="distribution-percent" title="Return Distribution" />
    </template>

    <!--Simulation Configuration-->
    <div v-else class="flex items-center justify-center min-h-72">
      <div class="w-full max-w-xl space-y-5 bg-indigo-200/10 bg-opacity-100 border border-border rounded-xl p-6">
        <p class="text-text-secondary text-md ">Configure and run the Monte Carlo simulation.</p>

        <div class="grid grid-cols-2 gap-3">
          <div>
            <label class="flex items-center gap-1 text-xs text-text-secondary mb-1">
              Horizon (days)
              <TooltipComp content="Number of trading days to simulate into the future." />
            </label>
            <input
              v-model.number="simConfig.horizon_days"
              type="number"
              min="1"
              class="w-full bg-transparent border-b border-border px-1 py-1.5 text-text-primary text-sm focus:outline-none focus:border-accent transition-colors"
            />
          </div>
          <div>
            <label class="flex items-center gap-1 text-xs text-text-secondary mb-1">
              Simulations
              <TooltipComp content="Number of simulation paths to generate. More paths yield more accurate results." />
            </label>
            <input
              v-model.number="simConfig.n_sims"
              type="number"
              min="1"
              class="w-full bg-transparent border-b border-border px-1 py-1.5 text-text-primary text-sm focus:outline-none focus:border-accent transition-colors"
            />
          </div>
          <div >
            <label class="flex items-center gap-1 text-xs text-text-secondary mb-1">
              Drift Scenario
              <TooltipComp content="Controls the expected return assumption. Shrinkage reduces historical drift towards zero to avoid overfitting." />
            </label>
            <select v-model="simConfig.drift_scenario" class="w-full bg-transparent border-b border-border px-1 py-1.5 text-text-primary text-sm focus:outline-none focus:border-accent transition-colors">
              <option value="SHRINKAGE_25">Shrinkage 25%</option>
              <option value="ZERO">Zero</option>
              <option value="HISTORICAL">Historical</option>
              <option value="RISK_FREE">Risk Free</option>
              </select>
        </div>
        <div>
          <label class="flex items-center gap-1 text-xs text-text-secondary mb-1">
            Volatility Scenario
            <TooltipComp content="Controls how volatility is estimated. EWMA weights recent observations more heavily; the percentage controls the blend with historical estimates." />
          </label>
          <select v-model="simConfig.volatility_scenario" class="w-full bg-transparent border-b border-border px-1 py-1.5 text-text-primary text-sm focus:outline-none focus:border-accent transition-colors">
            <option value="HISTORICAL">Historical</option>
            <option value="EMWA_100">EWMA 100%</option>
            <option value="EMWA_75">EWMA 75%</option>
            <option value="EMWA_50">EWMA 50%</option>
            </select>
        </div>
        <div>
          <label class="flex items-center gap-1 text-xs text-text-secondary mb-1">
            Multivariate-t Distribution
            <TooltipComp content="When enabled, uses a multivariate-t distribution to better capture fat tails and extreme market events." />
          </label>
          <select v-model="simConfig.multivariate_t" class="w-full bg-transparent border-b border-border px-1 py-1.5 text-text-primary text-sm focus:outline-none focus:border-accent transition-colors">
            <option value="ENABLED">Enabled</option>
            <option value="DISABLED">Disabled</option>
            </select>
        </div>
        <div>
          <label class="flex items-center gap-1 text-xs text-text-secondary mb-1">
            Regimes
            <TooltipComp content="When enabled, uses different market regimes to better capture varying market conditions." />
          </label>
          <select v-model="simConfig.regimes" class="w-full bg-transparent border-b border-border px-1 py-1.5 text-text-primary text-sm focus:outline-none focus:border-accent transition-colors">
            <option value="ENABLED">Enabled</option>
            <option value="DISABLED">Disabled</option>
            </select>
        </div>

        <!-- Backtest parameters -->
        <div class="col-span-2 border-t border-border pt-3 -mb-1">
          <span class="text-xs font-semibold uppercase tracking-wider text-text-secondary">Backtest</span>
        </div>
        <div>
          <label class="flex items-center gap-1 text-xs text-text-secondary mb-1">
            Testing Periods
            <TooltipComp content="Number of walk-forward windows used in the backtest." />
          </label>
          <input
            v-model.number="simConfig.n_testings"
            type="number"
            min="1"
            class="w-full bg-transparent border-b border-border px-1 py-1.5 text-text-primary text-sm focus:outline-none focus:border-accent transition-colors"
          />
        </div>
        <div>
          <label class="flex items-center gap-1 text-xs text-text-secondary mb-1">
            Period Length (days)
            <TooltipComp content="Length of each walk-forward testing window in trading days." />
          </label>
          <input
            v-model.number="simConfig.testing_period"
            type="number"
            min="1"
            class="w-full bg-transparent border-b border-border px-1 py-1.5 text-text-primary text-sm focus:outline-none focus:border-accent transition-colors"
          />
        </div>
        </div>

        <button
          @click="start_simulation"
          class="w-full bg-accent/70 hover:bg-accent-hover text-white text-sm font-medium rounded-lg py-2 transition-colors cursor-pointer"
        >
          Run Simulation
        </button>
      </div>
    </div>
  </div>
  
</template>