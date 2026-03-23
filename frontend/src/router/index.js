import { createRouter, createWebHistory } from 'vue-router'
import MyPortfolio from '@/views/MyPortfolio.vue'
import SimulationView from '@/views/SimulationView.vue'
import AssetExplorer from '@/views/AssetExplorer.vue'
import Backtest from '@/views/Backtest.vue'

const router = createRouter({
  history: createWebHistory(import.meta.env.BASE_URL),
  routes: [
    { path: '/', name: 'portfolio', component: MyPortfolio },
    { path: '/simulation', name: 'simulation', component: SimulationView },
    { path: '/explorer', name: 'explorer', component: AssetExplorer },
    { path: '/backtest', name: 'backtest', component: Backtest},
  ],
})

export default router
