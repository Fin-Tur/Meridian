const API_BASE = import.meta.env.VITE_API_BASE_URL

import { checkResponse } from '@/services/utils.js'

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

// ── Dummy data helpers ──
function randomNormal(mean = 0, std = 1) {
  const u1 = Math.random()
  const u2 = Math.random()
  return mean + std * Math.sqrt(-2 * Math.log(u1)) * Math.cos(2 * Math.PI * u2)
}

function delay(ms = 200) {
  return new Promise(r => setTimeout(r, ms))
}

// Runs monte carlo simulation, returns 50 bin values (for a histogram), binwidth, avg return and var95, var99, cvar95, cvar99, min, max
export async function fetch_simulation() {
  // TODO: replace with  return await api_call('/simulation')
  await delay(300)
  const bins = []
  for (let i = 0; i < 50; i++) {
    bins.push(Math.max(0, Math.round(400 * Math.exp(-0.5 * Math.pow((i - 25) / 8, 2)) + randomNormal(0, 15))))
  }
  return {
    bins,
    bin_width: 1.2,
    avg_return: 7.42,
    var95: -12.35,
    var99: -18.71,
    cvar95: -15.88,
    cvar99: -22.14,
    min: -32.5,
    max: 45.8,
  }
}

// Fetches correlation data for the assets in the portfolio, returns a matrix of correlation values and a list of asset symbols
export async function fetch_correlation() {
  // TODO: replace with  return await api_call('/correlation')
  await delay(200)
  const symbols = ['AAPL', 'MSFT', 'GOOGL', 'AMZN', 'TSLA']
  const n = symbols.length
  const matrix = []
  for (let i = 0; i < n; i++) {
    const row = []
    for (let j = 0; j < n; j++) {
      if (i === j) row.push(1.0)
      else if (j < i) row.push(matrix[j][i])
      else row.push(parseFloat((Math.random() * 1.4 - 0.2).toFixed(2)))
    }
    matrix.push(row)
  }
  return { symbols, matrix }
}

// Fetches portfolio infos like total value, individual asset values, returns, etc. Returns a list of assets with their symbol, value, return, and weight in the portfolio aswell as their correlation between assets
export async function fetch_portfolio() {
  // TODO: replace with  return await api_call('/portfolio')
  await delay(250)
  const assets = [
    { symbol: 'AAPL',  value: 28500,  return: 12.4,  weight: 0.30 },
    { symbol: 'MSFT',  value: 22100,  return: 8.7,   weight: 0.23 },
    { symbol: 'GOOGL', value: 17800,  return: -3.2,  weight: 0.19 },
    { symbol: 'AMZN',  value: 15200,  return: 15.1,  weight: 0.16 },
    { symbol: 'TSLA',  value: 11400,  return: -7.8,  weight: 0.12 },
  ]
  const total_value = assets.reduce((s, a) => s + a.value, 0)
  const total_return = assets.reduce((s, a) => s + a.return * a.weight, 0)
  return { total_value, total_return, assets }
}

// Fetches asset data : symbol, currency, adj_closes, volatility, avg_log_return, sharpe_ratio, max_drawdown, ytd_returns, skewness, kurtosis
export async function fetch_asset(symbol) {
  // TODO: replace with  return await api_call(`/asset?symbol=${encodeURIComponent(symbol)}`)
  await delay(350)
  const closes = []
  let price = 150 + Math.random() * 100
  for (let i = 0; i < 252; i++) {
    price *= 1 + randomNormal(0.0003, 0.018)
    closes.push(parseFloat(price.toFixed(2)))
  }
  return {
    symbol: symbol.toUpperCase(),
    currency: 'USD',
    adj_closes: closes,
    volatility: 0.18 + Math.random() * 0.12,
    avg_log_return: 0.0003 + Math.random() * 0.001,
    sharpe_ratio: 0.4 + Math.random() * 1.2,
    max_drawdown: -(0.08 + Math.random() * 0.25),
    ytd_returns: -0.05 + Math.random() * 0.35,
    skewness: -0.3 + Math.random() * 0.6,
    kurtosis: 2.5 + Math.random() * 3,
  }
}