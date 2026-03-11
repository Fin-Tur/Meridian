const API_BASE = import.meta.env.VITE_API_BASE_URL

import { checkResponse } from '@/services/utils.js'
import { usePortfolioStore } from '@/stores/counter'

async function api_call(endpoint, method = 'GET', body = null) {
  try {
    const options = { method }
    
    if (body) {
      if (body instanceof FormData) {
        options.body = body
      } else {
        options.headers = { ...options.headers, 'Content-Type': 'application/json' }
        options.body = JSON.stringify(body)
      }
    }
    const response = await fetch(`${API_BASE}${endpoint}`, options)
    const data = await checkResponse(response)
    if (data.success) {
      return data.data
    } else {
      alert('API call failed: ' + data.status)
      return null
    }
  } catch (error) {
    alert('API call failed: ' + error.message)
    console.error('Error during API call:', error) //Debug
    return null
  }
 
}

// ── Helpers ──

function lastOf(arr, fallback = 0) {
  return arr && arr.length > 0 ? arr[arr.length - 1] : fallback
}

// Computes daily log returns from an array of prices
function computeLogReturns(closes) {
  if (!closes || closes.length < 2) return []
  return closes.slice(1).map((p, i) => Math.log(p / closes[i]))
}

// Runs monte carlo simulation for the current portfolio.
// Returns histogram bins, labels, avg_return, min, max, var95, var99, cvar95, cvar99.
export async function fetch_simulation() {
  const store = usePortfolioStore()
  const portfolioAssets = store.assets

  if (!portfolioAssets.length) return null

  const assetDataList = await Promise.all(
    portfolioAssets.map(a => api_call('/asset', 'POST', { ticker: a.symbol, type: a.type }))
  )

  const enriched = portfolioAssets.map((a, i) => {
    const closes = assetDataList[i]?.adj_closes ?? []
    const price = lastOf(closes)
    return { symbol: a.symbol, type: a.type || 'stock', totalValue: price * a.quantity }
  })


  const raw = await api_call('/simulate', 'POST', {
    portfolio_value,
    horizon_days: 252,
    n_simulations: 10000,
    assets,
  })

  if (!raw) return null

  // Convert absolute dollar values to fractional returns relative to starting portfolio value
  const toReturn = val => portfolio_value > 0 ? (val - portfolio_value) / portfolio_value : 0

  // Build histogram labels from min value + bin_width steps
  const labels = raw.histogram_bins.map((_, i) =>
    '$' + (raw.min + i * raw.bin_width).toLocaleString(undefined, { maximumFractionDigits: 0 })
  )

  return {
    avg_return: toReturn(raw.avg),
    min: toReturn(raw.min),
    max: toReturn(raw.max),
    var95: raw.var_95,
    var99: raw.var_99,
    cvar95: raw.cvar_95,
    cvar99: raw.cvar_99,
    bins: raw.histogram_bins,
    labels,
  }

}

// Fetches correlation matrix for the current portfolio assets.
// Returns { symbols: string[], matrix: number[][] }
export async function fetch_correlation() {
  const store = usePortfolioStore()
  const portfolioAssets = store.assets

  if (!portfolioAssets.length) return null

  const assets = portfolioAssets.map(a => ({ ticker: a.symbol, type: a.type || 'stock' }))
  const raw = await api_call('/correlation', 'POST', { assets })

  if (!raw) return null

  const symbols = Object.keys(raw)
  const matrix = symbols.map(s1 => symbols.map(s2 => raw[s1][s2]))

  return { symbols, matrix }
}

// Fetches portfolio data for all assets in the store.
// Returns { total_value, total_return, assets: [{ symbol, value, return, weight }] }
export async function fetch_portfolio() {
  const store = usePortfolioStore()
  const assets = store.assets

  if (!assets.length) return { total_value: 0, total_return: 0, assets: [] }

  const res = await api_call('/portfolio', 'POST', {
    assets: assets.map(a => ({ ticker: a.symbol, amount: a.quantity, type: a.type}))
  })
  
  return res
}

// Fetches data for a single asset by ticker symbol.
// Returns server fields plus ytd_returns alias and computed log_returns.
export async function fetch_asset(symbol, type = 'stock') {
  const raw = await api_call('/asset', 'POST', { ticker: symbol.toUpperCase(), type })

  if (!raw) return null

  return {
    ...raw,
    ytd_returns: raw.ytd_return,                    // alias for view compatibility
    log_returns: computeLogReturns(raw.adj_closes),  // derived for histogram
  }
}