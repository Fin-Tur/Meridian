<script setup>
import { ref } from 'vue'
import { usePortfolioStore } from '@/stores/portfolio.js'
import { fetchBacktestData } from '@/services/api.js'
import { useSimConfigStore } from '@/stores/sim_config.js'
import InfoCard from '@/components/InfoCard.vue'
import BackArrow from '@/components/BackArrow.vue'
import UnitToggle from '@/components/UnitToggle.vue'
import BacktestChart from '@/components/BacktestChart.vue'

const portfolio = usePortfolioStore()
const simConfig = useSimConfigStore()
const backtestData = ref(null)
const loading = ref(false)
const error = ref(null)
const display_unit = ref('$')

async function loadData() {
    loading.value = true
    error.value = null
    try {
        const data = await fetchBacktestData(portfolio, simConfig)
        backtestData.value = data
    } catch (err) {
        error.value = err.message || 'Failed to load backtest data.'
    } finally {
        loading.value = false
    }
}

function resetBacktest() {
    backtestData.value = null
    error.value = null
    loading.value = false
}

const driftLabel = { SHRINKAGE_25: 'Shrinkage 25%', ZERO: 'Zero', HISTORICAL: 'Historical', RISK_FREE: 'Risk Free' }
const volLabel   = { HISTORICAL: 'Historical', EMWA_100: 'EWMA 100%', EMWA_75: 'EWMA 75%', EMWA_50: 'EWMA 50%' }
</script>

<template>
  <div class="max-w-6xl mx-auto px-6 py-8">

    <!-- Header -->
    <div class="flex items-center justify-between mb-6">
      <div class="relative flex items-center">
        <BackArrow v-if="backtestData" @back="resetBacktest" class="absolute right-full mr-6" />
        <h1 class="text-2xl font-bold">Backtest</h1>
      </div>
      <UnitToggle v-if="backtestData" v-model="display_unit" />
    </div>

    <!-- Loading -->
    <div v-if="loading" class="flex items-center justify-center min-h-72 text-text-secondary">
      <div class="spinner"></div>
      Running backtest…
    </div>

    <!-- Error -->
    <div v-else-if="error && !backtestData" class="flex flex-col items-center justify-center min-h-72 gap-4">
      <p class="text-negative text-sm">{{ error }}</p>
      <button
        @click="resetBacktest"
        class="px-4 py-2 text-sm font-medium rounded-lg border border-border text-text-secondary hover:text-text-primary hover:border-accent transition-colors cursor-pointer"
      >
        Try Again
      </button>
    </div>

    <!-- Results -->
    <template v-else-if="backtestData">

      <!-- Row 1 — General Stats -->
      <div class="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-4 gap-4 mb-4">
        <InfoCard title="Portfolio Value" :val="portfolio.portfolio_value" type="currency" :decimals="2" />
        <InfoCard
          v-if="display_unit === '$'"
          title="Avg Estimated Return Error %"
          :val="(backtestData.avg_return_diff * portfolio.portfolio_value ?? 0)"
          type="currency"
          :decimals="2"
          tooltip="Average estimated return error across all testing periods."
        />
        <InfoCard
          v-else
          title="Avg Estimated Return Error %"
          :val="(backtestData.avg_return_diff  ?? 0)"
          type="percentile"
          :decimals="2"
          tooltip="Average estimated return error across all testing periods."
        />
        
        <InfoCard
          v-if="display_unit === '$'"
          title="Median Estimated Return Error %"
          :val="(backtestData.median_return_diff * portfolio.portfolio_value ?? 0)"
          type="currency"
          :decimals="2"
          tooltip="Median estimated return error across all testing periods."
        />
        <InfoCard
          v-else
          title="Median Estimated Return Error %"
          :val="(backtestData.median_return_diff  ?? 0)"
          type="percentile"
          :decimals="2"
          tooltip="Median estimated return error across all testing periods."
        />
      </div>

      <!-- Row 2 — Risk Metrics -->
      <div class="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-4 gap-4 mb-6">
        <InfoCard
          title="VaR 95% Breach Rate"
          :val="backtestData.exceedance_rate_95 ?? 0"
          type="percentile"
          :decimals="2"
          tooltip="Percentage of periods where the actual loss exceeded the predicted VaR 95%. Expected: 5%. Lower values indicate a more conservative model, while higher values indicate a more optimistic model."
        />
        <InfoCard
          title="VaR 99% Breach Rate"
          :val="backtestData.exceedance_rate_99 ?? 0"
          type="percentile"
          :decimals="2"
          tooltip="Percentage of periods where the actual loss exceeded the predicted VaR 99%. Expected: 1%. Lower values indicate a more conservative model, while higher values indicate a more optimistic model."
        />
        <InfoCard
          title="Christoffersen Test p-value"
          :val="backtestData.christoffersen_pass ? backtestData.christoffersen_lr : -backtestData.christoffersen_lr"
          type="beneficial"
          :decimals="2"
          :tooltip="`Christoffersen's test for correct conditional coverage of regimes. Clustering in VaR Exceedances lead to higher absolute p-values, while independence leads to lower  absolute p-values.`"
        />
      </div>

      <!-- Chart -->
      <BacktestChart
        v-if="backtestData.actual_portfolio_value"
        :actual-values="backtestData.actual_portfolio_value"
        :simulated-values="backtestData.simulated_portfolio_value"
        :exceedances95="backtestData.exceedances_95"
        :exceedances99="backtestData.exceedances_99"
      />

    </template>

    <!-- Start Panel -->
    <div v-else class="flex items-center justify-center min-h-72">
      <div class="w-full max-w-xl space-y-5 bg-indigo-200/10 border border-border rounded-xl p-6">

        <div>
          <h2 class="text-base font-semibold text-text-primary mb-1">Walk-Forward Backtest</h2>
          <p class="text-text-secondary text-sm">
            Tests your portfolio against historical data using the current simulation parameters.
          </p>
        </div>

        <!-- Config Summary -->
        <div class="grid grid-cols-2 gap-x-6 gap-y-2 text-sm border-t border-border pt-4">
          <div class="flex justify-between">
            <span class="text-text-secondary">Testing Periods</span>
            <span class="text-text-primary font-medium">{{ simConfig.n_testings }}</span>
          </div>
          <div class="flex justify-between">
            <span class="text-text-secondary">Period Length</span>
            <span class="text-text-primary font-medium">{{ simConfig.testing_period }} days</span>
          </div>
          <div class="flex justify-between">
            <span class="text-text-secondary">Drift Scenario</span>
            <span class="text-text-primary font-medium">{{ driftLabel[simConfig.drift_scenario] ?? simConfig.drift_scenario }}</span>
          </div>
          <div class="flex justify-between">
            <span class="text-text-secondary">Volatility</span>
            <span class="text-text-primary font-medium">{{ volLabel[simConfig.volatility_scenario] ?? simConfig.volatility_scenario }}</span>
          </div>
          <div class="flex justify-between">
            <span class="text-text-secondary">Multivariate-t</span>
            <span class="text-text-primary font-medium">{{ simConfig.multivariate_t }}</span>
          </div>
          <div class="flex justify-between">
            <span class="text-text-secondary">Regimes</span>
            <span class="text-text-primary font-medium">{{ simConfig.regimes }}</span>
          </div>
        </div>

        <p class="text-xs text-text-secondary border-t border-border pt-3">
          Parameters can be adjusted in the
          <RouterLink to="/simulation" class="text-accent hover:underline">Simulation</RouterLink>
          settings.
        </p>

        <button
          @click="loadData"
          class="w-full bg-accent/70 hover:bg-accent-hover text-white text-sm font-medium rounded-lg py-2 transition-colors cursor-pointer"
        >
          Run Backtest
        </button>
      </div>
    </div>

  </div>
</template>