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
4. [API Reference](#3-api-reference)
5. [Build & Setup](#4-build--setup)
7. [View](#5-view)

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

*coming soon*

---

## 4. API Reference

> *coming soon*

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



