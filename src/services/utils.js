// More status interceptions!
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
  } else if (response.status === 404) {
    return { success: true, data: null }
  }
  const exceptionDTO = await response.json()
  return { success: false, status: response.status, error: exceptionDTO }
}