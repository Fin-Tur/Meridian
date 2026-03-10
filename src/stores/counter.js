import { ref, computed } from 'vue'
import { defineStore } from 'pinia'

export const usePortfolioStore = defineStore('portfolio', () => {
  const assets = ref([
    { symbol: 'AAPL', quantity: 15 },
    { symbol: 'MSFT', quantity: 10 },
    { symbol: 'GOOGL', quantity: 5 },
    { symbol: 'AMZN', quantity: 8 },
    { symbol: 'TSLA', quantity: 12 },
  ])

  const symbols = computed(() => assets.value.map(a => a.symbol))

  function addAsset(symbol, quantity) {
    const sym = symbol.trim().toUpperCase()
    if (!sym || quantity <= 0) return
    const existing = assets.value.find(a => a.symbol === sym)
    if (existing) {
      existing.quantity += quantity
    } else {
      assets.value.push({ symbol: sym, quantity })
    }
  }

  function removeAsset(symbol) {
    const index = assets.value.findIndex(a => a.symbol === symbol)
    if (index !== -1) {
      assets.value.splice(index, 1)
    }
  }

  return { assets, symbols, addAsset, removeAsset }
})
