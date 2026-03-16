// More status interceptions!
import {ref, watch} from 'vue'

export async function checkResponse(response) {
  if (!response) {
    console.error('No response received') //Debug
    return {
      success: false,
      status: 'No response',
      error: 'No response received from API',
    }
  }
  if (!(response instanceof Response)) {
    console.error('Invalid response object:', response) //Debug
    return {
      success: false,
      status: 'Invalid response',
      error: 'Response is not a valid Response object',
    }
  }
  if (response.ok) {
    return { success: true, data: await response.json() }
  }
  const exceptionDTO = await response.json()
  return { success: false, status: response.status, error: exceptionDTO }
}

export function formatCurrency(currency, value) {
  if(isNaN(value)) return 'N/A'
  if(currency === 'USD') {
    return "$" + value.toLocaleString(undefined, { minimumFractionDigits: 2, maximumFractionDigits: 2 })
  }
  return new Intl.NumberFormat(undefined, { style: 'currency', currency }).format(value)
}

export function useAnimatedCounter(target, options = {}) {
  const {
    duration = 700,
    steps = 40,
    immediate = true
  } = options
  const value = ref(0)

  function animate(to) {
    if (to == null) return
    const increment = to / steps
    let current = 0
    value.value = 0
    const interval = setInterval(() => {
      current += increment
      const done = to >= 0 ? current >= to : current <= to
      if (done) {
        value.value = to
        clearInterval(interval)
      } else {
        value.value = current
      }
    }, duration / steps)
  }

  if (immediate) animate(target)
  watch(() => target, (to) => animate(to))

  return value
}