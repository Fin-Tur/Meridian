# Monte Carlo Simulation — Mathematischer Ablauf

## 1. Eingangsdaten

Für jedes Asset $i$ werden historische, adjustierte Schlusskurse $P_i^{(0)}, P_i^{(1)}, \ldots, P_i^{(N)}$ verwendet.

---

## 2. Statistische Kenngrößen pro Asset

### 2.1 Logarithmische Renditen

Alle Berechnungen basieren auf täglichen Log-Renditen:

$$r_i^{(t)} = \ln\!\left(\frac{P_i^{(t)}}{P_i^{(t-1)}}\right)$$

### 2.2 Mittlere Log-Rendite $\mu$

$$\mu_i = \frac{1}{N} \sum_{t=1}^{N} r_i^{(t)}$$

### 2.3 Historische Volatilität $\sigma$

Stichproben-Standardabweichung der Log-Renditen:

$$\sigma_i = \sqrt{\frac{1}{N-1} \sum_{t=1}^{N} \left(r_i^{(t)} - \mu_i\right)^2}$$

### 2.4 Excess Kurtosis $\kappa$

$$\kappa_i = \frac{1}{N-1} \sum_{t=1}^{N} \left(\frac{r_i^{(t)} - \mu_i}{\sigma_i}\right)^4 - 3$$

Die Subtraktion von 3 entfernt die Normalkurtosis — ein Wert $\kappa > 0$ signalisiert fettere Schwänze als die Normalverteilung.

### 2.5 Freiheitsgrade $\nu$ der Student-t-Verteilung

Aus der theoretischen Beziehung zwischen Excess Kurtosis und den Freiheitsgraden einer Student-t-Verteilung:

$$\kappa = \frac{6}{\nu - 4} \quad \Longrightarrow \quad \nu = \frac{6}{\kappa} + 4$$

Ist $\nu \leq 4$ (d.h. $\kappa \leq 0$), wird $\nu = \infty$ gesetzt, was der Normalverteilung entspricht. Zusätzlich wird $\nu \geq 5$ erzwungen, um numerische Stabilität zu garantieren.

---

## 3. Kovarianzstruktur

### 3.1 Empirische Kovarianzmatrix

Nur zeitlich übereinstimmende Handelstage (Timestamp-Alignment) beider Assets werden berücksichtigt. Für ein Paar $(i, j)$ mit gemeinsamen Renditen $\{(r_i^{(k)}, r_j^{(k)})\}_{k=1}^{M}$:

$$\Sigma_{ij} = \frac{1}{M-1} \sum_{k=1}^{M} \left(r_i^{(k)} - \bar{\mu}_i\right)\left(r_j^{(k)} - \bar{\mu}_j\right)$$

Die resultierende Matrix $\Sigma \in \mathbb{R}^{n \times n}$ ist symmetrisch und positiv semidefinit.

### 3.2 Normalisierung zur Korrelationsmatrix

$$\rho_{ij} = \frac{\Sigma_{ij}}{\sqrt{\Sigma_{ii} \cdot \Sigma_{jj}}}$$

Die Diagonale von $\rho$ ist damit identisch 1. Diese Korrelationsmatrix wird als Basis für die Simulation verwendet.

### 3.3 Cholesky-Zerlegung

Die Korrelationsmatrix wird Cholesky-faktorisiert:

$$\rho = L \cdot L^\top, \quad L \in \mathbb{R}^{n \times n} \text{ (untere Dreiecksmatrix)}$$

$L$ wird benötigt, um unkorrelierte Zufallsvektoren in korrelierte zu transformieren.

---

## 4. Monte-Carlo-Simulation (Parallelisiert)

### 4.1 Geometrische Brownsche Bewegung mit Student-t-Innovationen

Die Preispfade folgen einer diskreten GBM-Dynamics. Für jeden Simulationstag $d$ und jedes Asset $j$:

**Schritt 1 — Unkorrelierte Student-t-Stichprobe:**

$$t_j \sim \text{Student-t}(\nu_j)$$

**Schritt 2 — Varianz-Normalisierung:**

Die Student-t-Verteilung hat Varianz $\frac{\nu}{\nu-2}$, nicht 1. Um mit $\sigma_j$ korrekt zu skalieren, wird auf Varianz 1 normiert:

$$Z_j = t_j \cdot \sqrt{\frac{\nu_j - 2}{\nu_j}}$$

**Schritt 3 — Korrelationsstruktur einbauen:**

$$\boldsymbol{\varepsilon} = L \cdot \mathbf{Z}, \quad \mathbf{Z} = (Z_1, \ldots, Z_n)^\top$$

Da $L L^\top = \rho$, gilt $\text{Cov}(\boldsymbol{\varepsilon}) = \rho$.

**Schritt 4 — Log-normaler Preisschritt:**

$$S_j^{(d+1)} = S_j^{(d)} \cdot \exp\!\left(\underbrace{\mu_j - \frac{\sigma_j^2}{2}}_{\text{Drift}} + \sigma_j \cdot \varepsilon_j\right)$$

Der Term $\mu_j - \frac{\sigma_j^2}{2}$ ist der Itô-Drift-Korrekturfaktor (Übergang von arithmetischer zu geometrischer Rendite). Er wird einmalig pro Simulation vorberechnet.

Die Startbedingung ist $S_j^{(0)} = 1$ (normiert). Nach `horizon_days` Schritten enthält $S_j$ den kumulativen Wachstumsfaktor des Assets.

### 4.2 Portfoliowert

Der finale Portfoliowert einer Simulation ergibt sich als gewichtete Summe der Wachstumsfaktoren:

$$V = \left(\mathbf{w} \cdot \mathbf{S}^{(\text{T})}\right) \cdot V_0$$

mit Gewichtsvektor $\mathbf{w}$ (Dezimalform, $\sum w_j = 1$) und initialem Portfoliowert $V_0$.

### 4.3 Parallelisierung

Die $N_{\text{sims}}$ Simulationen werden gleichmäßig auf $n_{\text{threads}}$ Threads verteilt. Jeder Thread berechnet `load_size` $= \lfloor N_{\text{sims}} / n_{\text{threads}} \rfloor$ Simulationen und schreibt direkt in disjunkte Bereiche des gemeinsamen Ergebnis-Arrays — ohne Synchronisation erforderlich. Verbleibende Simulationen ($N_{\text{sims}} \bmod n_{\text{threads}}$) werden im Hauptthread ausgeführt.

---

## 5. Risikomaße

Die $N_{\text{sims}}$ finalen Portfoliowerte werden aufsteigend sortiert. Sei $V_{(\alpha)}$ der $\alpha$-Quantilwert (Index $\lfloor \alpha \cdot N_{\text{sims}} \rfloor$).

### 5.1 Value at Risk (VaR)

Maximaler Verlust, der mit Konfidenz $(1-\alpha)$ nicht überschritten wird:

$$\text{VaR}_\alpha = V_0 - V_{(\alpha)}$$

Berechnet für $\alpha = 5\%$ (95%-Konfidenz) und $\alpha = 1\%$ (99%-Konfidenz).

### 5.2 Conditional Value at Risk (CVaR / Expected Shortfall)

Erwarteter Verlust gegeben, dass der Verlust die VaR überschreitet:

$$\text{CVaR}_\alpha = V_0 - \frac{1}{\lfloor \alpha \cdot N_{\text{sims}} \rfloor} \sum_{k=1}^{\lfloor \alpha \cdot N_{\text{sims}} \rfloor} V_{(k)}$$

CVaR ist ein kohärentes Risikomaß und sensitiver für Tail-Risiken als VaR.

### 5.3 Erwarteter Portfoliowert

$$\bar{V} = \frac{1}{N_{\text{sims}}} \sum_{k=1}^{N_{\text{sims}}} V_{(k)}$$
