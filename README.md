# Meridian

> Meridian analyzes your Portfolio in terms of risc and potential growth.

---

## Table of Contents

1. [Project Overview](#1-project-overview)
2. [Marcov Chain Monte Carlo Simulation — Mathematical Walkthrough](#2-monte-carlo-simulation--mathematical-walkthrough)
   - 2.1 [Input Data & Log Returns](#21-input-data--log-returns)
   - 2.2 [Statistical Parameters per Asset](#22-statistical-parameters-per-asset)
   - 2.3 [EWMA Volatility](#23-ewma-volatility)
   - 2.4 [Covariance Structure & Cholesky Decomposition](#24-covariance-structure--cholesky-decomposition)
   - 2.5 [Simulation Configuration](#25-simulation-configuration)
   - 2.6 [Market Regimes](#26-market-regimes)
   - 2.7 [Price Path Simulation (GBM)](#27-price-path-simulation-gbm)
   - 2.8 [Portfolio Value & Parallelization](#28-portfolio-value--parallelization)
   - 2.9 [Risk Measures](#29-risk-measures)
3. [Backtesting](#3-backtesting)
   - 3.1 [Settings](#31-settings)
   - 3.2 [Precomnpute](#32-precompute)
   - 3.3 [Testing](#33-testing-phase)
   - 3.4 [Result-Analysis](#34-result-analysis)
4. [API Reference](#4-api-reference)
5. [Build & Setup](#5-build--setup)
6. [View](#6-view)

---

## 1. Project Overview

> Meridian is a Software composed of an C++ Backend/API with a modern Vue.js Frontend. It combines valuable MCMC risc management analysis as well as an overwiev of your Portfolio (eg. Correlations, Composition). To verfiy this model for your own exact Asset-Stack it includes the option of Backtesting with premanagable assets and time horizons.

---

## 2. Marcov Chain Monte Carlo Simulation — Mathematical Walkthrough

The simulation models the stochastic evolution of a portfolio over a defined time horizon. It is based on **Geometric Brownian Motion (GBM)** with Student-t innovations, correlation structure via Cholesky decomposition, and an optional market regime-switching model.

---

### 2.1 Input Data & Log Returns

For each asset $i$, historical adjusted closing prices $P_i^{(0)}, P_i^{(1)}, \ldots, P_i^{(N)}$ are used.

All calculations are based on daily **log returns**:

$$r_i^{(t)} = \ln\!\left(\frac{P_i^{(t)}}{P_i^{(t-1)}}\right)$$

> *Note on timestamp alignment: only trading days common to all assets are used.*

---

### 2.2 Statistical Parameters per Asset

#### Mean Log Return $\mu$

$$\mu_i = \frac{1}{N} \sum_{t=1}^{N} r_i^{(t)}$$

#### Historical Volatility $\sigma$

Sample standard deviation of log returns:

$$\sigma_i = \sqrt{\frac{1}{N-1} \sum_{t=1}^{N} \left(r_i^{(t)} - \mu_i\right)^2}$$

#### Excess Kurtosis $\kappa$

$$\kappa_i = \frac{1}{N-1} \sum_{t=1}^{N} \left(\frac{r_i^{(t)} - \mu_i}{\sigma_i}\right)^4 - 3$$

A value of $\kappa > 0$ indicates heavier tails than the normal distribution (*fat tails*).

#### Degrees of Freedom $\nu$ of the Student-t Distribution

From the theoretical relationship between excess kurtosis and degrees of freedom:

$$\kappa = \frac{6}{\nu - 4} \quad \Longrightarrow \quad \nu = \frac{6}{\kappa} + 4$$

Constraints: $\nu \geq 5$ (numerical stability). For longer horizons, $\nu$ converges toward the normal distribution:

$$\nu_{\text{eff}} = \nu + (30 - \nu) \cdot \min\!\left(1,\; \frac{T}{63}\right)$$

---

### 2.3 EWMA Volatility

To better reflect current market conditions, volatility is updated using **Exponentially Weighted Moving Average (EWMA)**, starting from $\sigma^2$:

$$\sigma_{\text{EWMA},t}^2 = \lambda \cdot \sigma_{\text{EWMA},t-1}^2 + (1 - \lambda) \cdot r_i^{(t)^2}$$

| Asset Class | $\lambda$ |
|---|---|
| Equities | $0.94$ *(JP Morgan RiskMetrics 1994)* |
| Cryptocurrencies | $0.90$ *(faster market dynamics)* |

In the `EWMA_75` and `EWMA_50` volatility models, $\sigma_{\text{EWMA}}$ is blended with the historical volatility $\sigma$:

$$\sigma_{\text{blend}} = \alpha \cdot \sigma_{\text{EWMA}} + (1 - \alpha) \cdot \sigma, \quad \alpha \in \{0.50,\; 0.75,\; 1.00\}$$

---

### 2.4 Covariance Structure & Cholesky Decomposition

#### Empirical Covariance Matrix

For an asset pair $(i, j)$ with $M$ common trading days:

$$\Sigma_{ij} = \frac{1}{M-1} \sum_{k=1}^{M} \left(r_i^{(k)} - \mu_i\right)\left(r_j^{(k)} - \mu_j\right)$$

The resulting matrix $\Sigma \in \mathbb{R}^{n \times n}$ is symmetric and positive semi-definite.

#### Normalization to Correlation Matrix

$$\rho_{ij} = \frac{\Sigma_{ij}}{\sqrt{\Sigma_{ii} \cdot \Sigma_{jj}}}$$

#### Stressed Covariance Matrix

> The Stressed Covariance Matrix is computed on a selected field of inputs. The assumption to justify this "extra-matrix" is the increased correlation between Assets in market crashes or crises. 
> The Requirements for a DataPoint to be selected for stress-correlation-matrix are the following: 
 > - Rolling 20 Day average of the equal weighted portfolio log return is in the bottom 10th percentile of all availible data.
 > - More then 70% of selected Assets have Data points for this day, and simultaneously show negative log returns.
> The Matrix is then computed as explained above.

#### Cholesky Decomposition

The correlation matrix (as well as the stressed and blended variants) is Cholesky-factorized:

$$\rho = L \cdot L^\top, \quad L \in \mathbb{R}^{n \times n} \text{ (lower triangular matrix)}$$

Separate $L$ matrices are precomputed per regime. The blended matrix for the ENERGIC regime:

$$\rho_{\text{blend}} = 0.6 \cdot \rho_{\text{calm}} + 0.4 \cdot \rho_{\text{stressed}}$$

---

### 2.5 Simulation Configuration

The simulation is controlled via a `sim_config` object with the following options:

#### Drift Scenarios

| Scenario | Effective Drift $\mu_j^{\text{eff}}$ | Description |
|---|---|---|
| `HISTORICAL` | $\mu_j$ | Historical mean log return |
| `ZERO` | $0$ | No drift — pure volatility simulation |
| `SHRINKAGE_25` | $0.25 \cdot \mu_j$ | Conservative drift with 75% shrinkage |
| `RISK_FREE` | $r_f / 252$ | Risk-free rate (~1% p.a.) |

#### Volatility Models

| Model | Volatility Used | Drift Adjustment |
|---|---|---|
| `HISTORICAL` | $\sigma_i$ | $\mu_i - \tfrac{1}{2}\sigma_i^2$ |
| `EWMA_100` | $\sigma_{\text{EWMA}}$ | $\mu_i - \tfrac{1}{2}\sigma_{\text{EWMA}}^2$ |
| `EWMA_75` | $0.75\,\sigma_{\text{EWMA}} + 0.25\,\sigma$ | $\mu_i - \tfrac{1}{2}\sigma_{\text{blend}}^2$ |
| `EWMA_50` | $0.50\,\sigma_{\text{EWMA}} + 0.50\,\sigma$ | $\mu_i - \tfrac{1}{2}\sigma_{\text{blend}}^2$ |

---

### 2.6 Market Regimes

The optional **regime-switching model** distinguishes three market phases that can change daily. Transitions are based on a uniformly drawn random number $u \sim \mathcal{U}(0,1)$:

| Regime | Characteristics |
|---|---|
| `CALM` | Normal correlation, low volatility |
| `ENERGIC` | Elevated correlation, medium volatility |
| `CRISIS` | Extreme correlation, high volatility |

#### Regime Transition Probabilities

| From \ To | CALM | ENERGIC | CRISIS |
|---|---|---|---|
| `CALM` | $u \geq 0.03$ | $0.002 \leq u < 0.03$ | $u < 0.002$ |
| `ENERGIC` | $u \geq 0.2$ | $0.05 \leq u < 0.2$ | $u < 0.05$ |
| `CRISIS` | $u \geq 0.35$ | $0.15 \leq u < 0.35$ | $u < 0.15$ |

#### Regime Parameter Multipliers

| Regime | $m_\sigma$ (Volatility) | $m_\nu$ (Degrees of Freedom) |
|---|---|---|
| `CALM` | $0.8$ | $1.5$ |
| `ENERGIC` | $1.4$ | $1.0$ |
| `CRISIS` | $2.5$ | $0.5$ |

A lower $m_\nu$ (fewer degrees of freedom) produces heavier tails in the crisis regime.

---

### 2.7 Price Path Simulation (GBM)

For each simulation day $d$ and each asset $j$, the following steps are performed:

#### Step 1 — Generate Random Shocks

**Variant A — Univariate Student-t (without `multivariate_t`):**

$$t_j \sim \text{Student-t}(\nu_j \cdot m_\nu)$$

Normalized to unit variance:

$$Z_j = t_j \cdot \sqrt{\frac{\nu_j \cdot m_\nu - 2}{\nu_j \cdot m_\nu}}$$

**Variant B — Multivariate t-Distribution (with `multivariate_t`):**

Sampling via a shared Chi² scaling factor:

$$\chi^2 \sim \chi^2(\nu_{\text{avg}} \cdot m_\nu), \quad s = \sqrt{\frac{\nu_{\text{avg}} \cdot m_\nu}{\chi^2}}$$

$$Z_j = \frac{\varepsilon_j}{s}, \quad \varepsilon_j \sim \mathcal{N}(0, 1)$$

This produces true multivariate t-distributed random vectors with joint tail dependence.

#### Step 2 — Introduce Correlation Structure

$$\boldsymbol{\varepsilon} = L_{\text{regime}} \cdot \mathbf{Z}, \quad \mathbf{Z} = (Z_1, \ldots, Z_n)^\top$$

Since $L L^\top = \rho$, it follows that $\text{Cov}(\boldsymbol{\varepsilon}) = \rho$.

#### Step 3 — Log-Normal Price Step (GBM)

$$S_j^{(d+1)} = S_j^{(d)} \cdot \exp\!\left(\underbrace{\mu_j^{\text{eff}} - \frac{\sigma_j^2}{2}}_{\text{Itô drift}} + \sigma_j \cdot m_\sigma \cdot \varepsilon_j\right)$$

The term $-\frac{\sigma_j^2}{2}$ is the **Itô correction term**, ensuring the correct transition from arithmetic to geometric returns. Initial condition: $S_j^{(0)} = 1$.

---

### 2.8 Portfolio Value & Parallelization

#### Final Portfolio Value

After $T$ simulation days, the final portfolio value is the weighted sum of asset growth factors:

$$V = \left(\mathbf{w}^\top \cdot \mathbf{S}^{(T)}\right) \cdot V_0$$

with weight vector $\mathbf{w}$ ($\sum_j w_j = 1$) and initial portfolio value $V_0$.

#### Parallelization

The $N_{\text{sims}}$ simulations are distributed evenly across $n_{\text{threads}}$ threads:

$$\text{load\_size} = \left\lfloor \frac{N_{\text{sims}}}{n_{\text{threads}}} \right\rfloor$$

Each thread writes into disjoint sections of the result array — no synchronization required. Remaining simulations ($N_{\text{sims}} \bmod n_{\text{threads}}$) are handled on the main thread.

---

### 2.9 Risk Measures

The $N_{\text{sims}}$ final portfolio values are sorted in ascending order. Let $V_{(k)}$ denote the $k$-th value in the sorted array.

#### Value at Risk (VaR)

Maximum loss not exceeded with probability $(1-\alpha)$:

$$\text{VaR}_\alpha = V_0 - V_{\left(\lfloor \alpha \cdot N_{\text{sims}} \rfloor\right)}$$

Computed for $\alpha = 5\%$ (95% confidence) and $\alpha = 1\%$ (99% confidence).

#### Conditional Value at Risk (CVaR / Expected Shortfall)

Expected loss given that the loss exceeds the VaR:

$$\text{CVaR}_\alpha = V_0 - \frac{1}{\lfloor \alpha \cdot N_{\text{sims}} \rfloor} \sum_{k=1}^{\lfloor \alpha \cdot N_{\text{sims}} \rfloor} V_{(k)}$$

CVaR is a **coherent risk measure** and more sensitive to tail risks than VaR.

#### Expected Portfolio Value & Median

$$\bar{V} = \frac{1}{N_{\text{sims}}} \sum_{k=1}^{N_{\text{sims}}} V_{(k)}, \qquad \tilde{V} = V_{\left(\lfloor N_{\text{sims}} / 2 \rfloor\right)}$$

---

## 3. Backtesting

The backtesting engine validates the Monte Carlo simulation model by running it against historical data in a **rolling-window** fashion. For each window, a simulation is executed using only data available *before* that window, and the predicted risk measures are compared against the actual realized portfolio returns.

---

### 3.1 Settings

A backtest job is configured with the following parameters:

| Parameter | Type | Description |
|---|---|---|
| `portfolio_value` | `double` | Initial portfolio value at the most recent window |
| `test_period` | `int` | Horizon in trading days per testing window |
| `n_testings` | `int` | Number of rolling test windows |
| `n_sims` | `size_t` | Number of Monte Carlo simulations per window |
| `config` | `sim_config` | Simulation configuration (drift scenario, volatility model, regimes, multivariate-t) |

The full simulation configuration (`sim_config`) is shared with the Monte Carlo engine — see [Section 2.5](#25-simulation-configuration).

---

### 3.2 Precompute

For each testing window $t \in \{0, \ldots, n_{\text{testings}}-1\}$, statistical parameters are precomputed on a **shrinking historical window** that excludes the future test period. Each successive window $t$ removes the most recent `test_period` data points:

$$N_t = N_0 - t \cdot \text{test\_period}$$

The following parameters are computed per window:

| Parameter | Symbol | Description |
|---|---|---|
| Mean log return | $\mu_t^{(a)}$ | Per-asset mean over $N_t$ data points |
| Historical volatility | $\sigma_t^{(a)}$ | Per-asset sample standard deviation |
| EWMA volatility | $\sigma_{\text{EWMA},t}^{(a)}$ | EWMA volatility with $\lambda$ depending on asset class |
| Degrees of freedom | $\nu_t^{(a)}$ | Per-asset Student-t df with horizon correction |
| Avg. degrees of freedom | $\bar{\nu}_t$ | Weight-averaged df (only if `multivariate_t` is enabled) |
| Cholesky matrices | $L_t^{(r)}$ | Per-regime Cholesky-decomposed correlation matrices |

Covariance matrices are recomputed adaptively:
- **Normal covariance matrix**: recomputed every **10** test periods
- **Stressed covariance matrix**: recomputed every **30** test periods (only when `regimes` is enabled)

Historical portfolio values are back-calculated for each window using actual asset growth:

$$V_t = \frac{V_{t-1}}{\sum_a w_a \cdot \frac{P_a^{(\text{end}_{t-1})}}{P_a^{(\text{start}_{t-1})}}}$$

---

### 3.3 Testing Phase

For each period $i \in \{1, \ldots, n_{\text{testings}}-1\}$:

1. **Simulate** — Run the full Monte Carlo simulation using the precomputed preset for window $i$
2. **Compute actual loss** — Calculate the realized portfolio return:
   $$L_i = -\frac{V_{i-1} - V_i}{V_i}$$
3. **Compare against VaR** — Check whether the actual loss exceeds the simulated VaR thresholds
4. **Track transitions** — Record consecutive exceedance transitions $(n_{00}, n_{01}, n_{10}, n_{11})$ for the Christoffersen test

---

### 3.4 Result Analysis

#### Exceedance Rates

$$\hat{p}_{95} = \frac{\sum_{i} \mathbb{1}[L_i > \text{VaR}_{95,i}]}{n_{\text{obs}}}, \qquad \hat{p}_{99} = \frac{\sum_{i} \mathbb{1}[L_i > \text{VaR}_{99,i}]}{n_{\text{obs}}}$$

A well-calibrated model should produce $\hat{p}_{95} \approx 0.05$ and $\hat{p}_{99} \approx 0.01$.

#### Christoffersen Independence Test

Tests whether VaR exceedances are **serially independent** using a likelihood ratio statistic:

$$LR = -2 \left[ (n_{00}+n_{10})\ln(1-\hat{p}) + (n_{01}+n_{11})\ln(\hat{p}) - n_{00}\ln(1-\hat{p}_{01}) - n_{01}\ln(\hat{p}_{01}) - n_{10}\ln(1-\hat{p}_{11}) - n_{11}\ln(\hat{p}_{11}) \right]$$

where:
- $\hat{p}_{01} = n_{01} / (n_{00} + n_{01})$ — probability of exceedance after a non-exceedance
- $\hat{p}_{11} = n_{11} / (n_{10} + n_{11})$ — probability of exceedance after an exceedance
- $\hat{p} = (n_{01} + n_{11}) / (n_{00} + n_{01} + n_{10} + n_{11})$ — unconditional exceedance probability

**Pass criterion:** $LR < 3.841$ ($\chi^2$ critical value at 95% confidence, 1 degree of freedom). A pass indicates that exceedances are independent — i.e. the model does not systematically cluster risk underestimation.

#### Median & Average Return Difference

The simulated median portfolio value is compared to the actual realized value per window. The median and average of these percentage differences across all windows provide a bias indicator for the model.

---

## 4. API Reference

The backend exposes a REST API via an embedded HTTP server (cpp-httplib). All endpoints accept and return `application/json`.

---

### `GET /health`

Health check endpoint.

**Response:**
```json
{ "status": "ok" }
```

---

### `POST /simulate`

Run a Monte Carlo simulation for a portfolio.

**Request Body:**

| Field | Type | Default | Description |
|---|---|---|---|
| `portfolio_value` | `number` | *required* | Total portfolio value |
| `horizon_days` | `number` | `252` | Simulation horizon in trading days |
| `n_simulations` | `number` | `1000` | Number of simulation paths |
| `drift_scenario` | `string` | `"SHRINKAGE_25"` | One of `HISTORICAL`, `ZERO`, `SHRINKAGE_25`, `RISK_FREE` |
| `volatility_model` | `string` | `"HISTORICAL"` | One of `HISTORICAL`, `EWMA_100`, `EWMA_75`, `EWMA_50` |
| `multivariate_t` | `bool` | `false` | Use multivariate t-distribution |
| `regimes` | `bool` | `false` | Enable regime-switching model |
| `assets` | `array` | *required* | Array of asset objects |

**Asset object:**

| Field | Type | Description |
|---|---|---|
| `ticker` | `string` | Asset ticker symbol (e.g. `"AAPL"`, `"BTC"`) |
| `type` | `string` | `"stock"` or `"crypto"` |
| `weight` | `number` | Portfolio weight (should sum to 1.0) |

**Response:**

| Field | Type | Description |
|---|---|---|
| `var_95` | `number` | Value at Risk at 95% confidence |
| `var_99` | `number` | Value at Risk at 99% confidence |
| `cvar_95` | `number` | Conditional VaR at 95% |
| `cvar_99` | `number` | Conditional VaR at 99% |
| `avg` | `number` | Mean simulated portfolio value |
| `median` | `number` | Median simulated portfolio value |
| `min` | `number` | Minimum simulated portfolio value |
| `max` | `number` | Maximum simulated portfolio value |
| `histogram_bins` | `number[]` | 50-bin histogram of final portfolio values |
| `bin_width` | `number` | Width of each histogram bin |

---

### `POST /correlation`

Compute the correlation matrix for a set of assets.

**Request Body:**

| Field | Type | Description |
|---|---|---|
| `assets` | `array` | Array of `{ "ticker": string, "type": string }` objects |

**Response:**

A nested JSON object mapping each ticker pair to its correlation coefficient:
```json
{
  "AAPL": { "AAPL": 1.0, "MSFT": 0.78 },
  "MSFT": { "AAPL": 0.78, "MSFT": 1.0 }
}
```

---

### `POST /asset`

Fetch market data and compute key statistics for a single asset.

**Request Body:**

| Field | Type | Description |
|---|---|---|
| `ticker` | `string` | Asset ticker symbol |
| `type` | `string` | `"stock"` or `"crypto"` |

**Response:**

| Field | Type | Description |
|---|---|---|
| `symbol` | `string` | Ticker symbol |
| `currency` | `string` | Price currency (e.g. `"USD"`) |
| `adj_closes` | `number[]` | Historical adjusted close prices |
| `volatility` | `number` | Annualized volatility ($\sigma \cdot \sqrt{252}$) |
| `avg_log_return` | `number` | Annualized mean log return ($\mu \cdot 252$) |
| `sharpe_ratio` | `number` | Sharpe ratio |
| `max_drawdown` | `number` | Maximum drawdown |
| `ytd_return` | `number` | Year-to-date return |
| `skewness` | `number` | Return distribution skewness |
| `kurtosis` | `number` | Return distribution kurtosis |

---

### `POST /portfolio`

Compute portfolio value and asset weights from holdings.

**Request Body:**

| Field | Type | Description |
|---|---|---|
| `assets` | `array` | Array of asset objects with `ticker`, `type`, and `amount` (number of units held) |

**Response:**

| Field | Type | Description |
|---|---|---|
| `portfolio_value` | `number` | Total portfolio value in USD |
| `weights` | `array` | Array of `[ticker, weight]` pairs |

> All asset prices are unified to USD via live FX rates before computing the portfolio value.

---

### `POST /backtest`

Run a rolling-window backtest of the Monte Carlo model.

**Request Body:**

| Field | Type | Default | Description |
|---|---|---|---|
| `portfolio_value` | `number` | *required* | Initial portfolio value |
| `testing_period` | `number` | `1` | Trading days per test window |
| `n_testings` | `number` | `100` | Number of rolling windows |
| `n_simulations` | `number` | `5000` | Simulations per window |
| `drift_scenario` | `string` | `"SHRINKAGE_25"` | Drift scenario |
| `volatility_model` | `string` | `"HISTORICAL"` | Volatility model |
| `multivariate_t` | `bool` | `false` | Use multivariate t-distribution |
| `regimes` | `bool` | `false` | Enable regime-switching |
| `assets` | `array` | *required* | Array of `{ "ticker", "type", "weight" }` objects |

**Response:**

| Field | Type | Description |
|---|---|---|
| `exceedance_rate_95` | `number` | Fraction of windows where actual loss exceeded VaR 95 |
| `exceedance_rate_99` | `number` | Fraction of windows where actual loss exceeded VaR 99 |
| `christoffersen_lr` | `number` | Christoffersen likelihood ratio statistic |
| `christoffersen_pass` | `bool` | `true` if exceedances are serially independent ($LR < 3.841$) |
| `median_return_diff` | `number` | Median simulated vs. actual return difference |
| `avg_return_diff` | `number` | Average simulated vs. actual return difference |
| `actual_portfolio_values` | `number[]` | Realized portfolio values per window |
| `exceedances_95` | `bool[]` | Per-window 95% exceedance flags |
| `exceedances_99` | `bool[]` | Per-window 99% exceedance flags |
| `simulated_portfolio_value` | `number[]` | Simulated median portfolio value per window |

---

### Data Sources

| Asset Class | Provider | Endpoint | Data |
|---|---|---|---|
| Stocks | Yahoo Finance | `query1.finance.yahoo.com/v8/finance/chart/` | Up to 10y daily adjusted closes |
| Crypto | Kraken | `api.kraken.com/0/public/OHLC` | Up to 1440 days daily OHLC candles |
| FX Rates | Yahoo Finance | `query1.finance.yahoo.com/v8/finance/chart/` | Live currency conversion rates |

All fetched data is cached in-memory for 1 day to reduce API calls.

---

## 5. Build & Setup

*Requirements:* Eigen,OpenSSL devel package

```bash
# Backend
cd backend
cmake -B build && cmake --build build

# Frontend
cd frontend
npm install && npm run dev
```

## 6. View

> My Portfolio
 <img width="1862" height="1134" alt="Screenshot From 2026-03-22 12-27-59" src="https://github.com/user-attachments/assets/372230a5-7e57-4cac-a350-78d68e44e074" />

> MCMC Simulation - Config
<img width="1442" height="695" alt="Screenshot From 2026-03-22 12-28-12" src="https://github.com/user-attachments/assets/cc79b786-5c85-4f27-b973-90b110b3d734" />

> MCMC Simulation - Results
<img width="1462" height="872" alt="Screenshot From 2026-03-22 12-28-27" src="https://github.com/user-attachments/assets/d4a5ae62-c52c-4a64-842c-ea4f94288dba" />

> Asset Explorer - View
<img width="1501" height="1112" alt="Screenshot From 2026-03-22 12-28-37" src="https://github.com/user-attachments/assets/2814d0de-20c1-4c8e-865c-1c3966c000f4" />

> Asset Explorer - Asset
<img width="1466" height="928" alt="Screenshot From 2026-03-22 12-28-45" src="https://github.com/user-attachments/assets/3de15db8-7eae-4eb6-ae16-ace5e07d105c" />

> Backtesting - Preset
<img width="980" height="488" alt="Screenshot 2026-03-25 105343" src="https://github.com/user-attachments/assets/4eee01b0-9fe4-4776-b9ee-ad4205c308d0" />

> Backtesting - Result
<img width="1167" height="708" alt="Screenshot 2026-03-25 105652" src="https://github.com/user-attachments/assets/7520847c-55cd-430b-85dc-7ecbaee01f2c" />


