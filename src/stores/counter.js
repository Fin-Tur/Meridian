import { ref, computed } from 'vue'
import { defineStore } from 'pinia'

export const usePortfolioStore = defineStore('portfolio', () => {

    const portfolio_value = ref(0)
    const weights = ref({}) // { symbol: weight }

  const assets = ref([
    { symbol: 'AAPL', quantity: 15, type: 'stock' },
    { symbol: 'MSFT', quantity: 10, type: 'stock' },
    { symbol: 'GOOGL', quantity: 5, type: 'stock' },
    { symbol: 'AMZN', quantity: 8, type: 'stock' },
    { symbol: 'TSLA', quantity: 12, type: 'stock' },
  ])

  const symbols = computed(() => assets.value.map(a => a.symbol))

  function addAsset(symbol, quantity, type = 'stock') {
    const sym = symbol.trim().toUpperCase()
    if (!sym || quantity <= 0) return
    const existing = assets.value.find(a => a.symbol === sym)
    if (existing) {
      existing.quantity += quantity
    } else {
      assets.value.push({ symbol: sym, quantity, type })
    }
  }

  function removeAsset(symbol) {
    const index = assets.value.findIndex(a => a.symbol === symbol)
    if (index !== -1) {
      assets.value.splice(index, 1)
    }
  }

  function setPortfolioValue(value) {
    portfolio_value.value = value
  }

  function setWeight(symbol, weight) {
    if (weight < 0 || weight > 1) return
    weights.value[symbol] = weight
  }

  function pctOfPortfolio(usd) {
    return portfolio_value.value > 0 ? (usd / portfolio_value.value) * 100 : 0
  }

  return { assets, symbols, portfolio_value, weights, setPortfolioValue, addAsset, removeAsset, setWeight, pctOfPortfolio }
})
