import { ref, computed } from 'vue'
import { defineStore } from 'pinia'

export const useSimConfigStore = defineStore('simConfig', () => {

    const n_sims = ref(5000)
    const horizon_days = ref(252)
    const n_testings = ref(10)
    const testing_period = ref(100)
    const drift_scenario = ref('SHRINKAGE_25')
    const volatility_scenario = ref('EWMA_75')
    const multivariate_t = ref('ENABLED')
    const regimes = ref('ENABLED')

  return { n_sims, horizon_days, n_testings, testing_period, drift_scenario, volatility_scenario, multivariate_t, regimes }
})