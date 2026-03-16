# Monte Carlo Simulation — Nächste Verbesserungsstufen

Dieses Dokument baut direkt auf deinem bisherigen Algorithmus auf und beschreibt sinnvolle nächste Schritte, um das Modell **realistischer, robuster und aussagekräftiger** zu machen.  
Jeder Abschnitt enthält:

- **Warum** die Verbesserung sinnvoll ist
- **Was** sich mathematisch ändert
- **Wie** du sie praktisch umsetzt

---

## 1. Drift \(\mu\) entschärfen oder ersetzen

### Warum?

Die historische mittlere tägliche Log-Rendite

\[
\mu_i = \frac{1}{N} \sum_{t=1}^{N} r_i^{(t)}
\]

ist in kurzen und mittleren Stichproben sehr instabil. Schon kleine Schätzfehler in \(\mu\) wirken sich wegen der exponentiellen Aufzinsung stark auf die rechte Tail der Simulation aus.

Das führt oft zu einem **zu optimistischen Modell**:

- der Mittelwert der Endwerte wird zu hoch
- sehr große positive Pfade treten zu häufig auf
- VaR/CVaR wirken im Verhältnis „zu freundlich“

Für Risikomessung ist \(\mu\) auf kurzen Horizonten meist deutlich weniger wichtig als \(\sigma\) und die Tail-Modellierung.

### Was ändern?

Statt

\[
S_j^{(d+1)} = S_j^{(d)} \exp\left((\mu_j - \tfrac12 \sigma_j^2) + \sigma_j \varepsilon_j\right)
\]

kannst du eine der folgenden Varianten verwenden.

#### Variante A — Null-Drift

\[
\mu_j^{\text{eff}} = 0
\]

Dann wird simuliert mit:

\[
S_j^{(d+1)} = S_j^{(d)} \exp\left(- \tfrac12 \sigma_j^2 + \sigma_j \varepsilon_j\right)
\]

#### Variante B — Shrinkage-Drift

\[
\mu_j^{\text{eff}} = \lambda \mu_j, \quad 0 \le \lambda \le 1
\]

z. B.

\[
\lambda = 0.25
\]

Dann bleibt Information aus der Historie erhalten, aber deutlich konservativer.

#### Variante C — Drift = risikofreier Satz

Wenn du später eine „ökonomisch saubere“ Version willst:

\[
\mu_j^{\text{eff}} = r_f
\]

oder für Aktien leicht darüber.

### Wie umsetzen?

Im Preset zusätzlich einen Modus einführen:

- `historical`
- `zero`
- `shrunk`
- `risk_free`

Dann bei der Drift-Vorbereitung:

\[
\text{drift}_j = \mu_j^{\text{eff}} - \frac{\sigma_j^2}{2}
\]

---

## 2. Längere Historie für \(\sigma\), Korrelation und Kurtosis verwenden

### Warum?

Mit nur 3 Jahren Daily Data schätzt du ungefähr 750 Renditen. Das reicht zwar für einen ersten Prototyp, aber nicht besonders stabil für:

- Volatilität
- Korrelationen
- insbesondere Kurtosis / Tail-Verhalten

Kurtosis-Schätzungen sind notorisch verrauscht. Eine kleine Änderung in den Renditen kann die geschätzten Freiheitsgrade stark verschieben.

### Was ändern?

Verwende je nach Verfügbarkeit **10–20 Jahre tägliche Daten** für:

- \(\sigma\)
- Kovarianz/Korrelation
- Tail-Parameter

Für \(\mu\) sogar eher noch länger — oder gleich konservativ behandeln.

### Wie umsetzen?

Praktisch:

1. Lade pro Asset die längste verfügbare Serie
2. bilde eine gemeinsame Renditehistorie über die überlappenden Handelstage
3. schätze die Parameter auf dieser längeren Historie

Optional kannst du zwei Fenster trennen:

- **langes Fenster** für robuste Basisschätzung
- **kurzes Fenster** für aktuelle Marktanpassung

---

## 3. Zeitskalierung explizit machen: \(\Delta t\)

### Warum?

Dein Modell verwendet tägliche Log-Renditen. Das ist okay, solange **alle Parameter ebenfalls täglich** sind.  
Sobald du annualisierte Größen nutzt, musst du \(\Delta t\) explizit berücksichtigen.

### Mathematisch

Die diskrete GBM-Form ist allgemein:

\[
S_{t+\Delta t} = S_t \exp\left((\mu - \tfrac12 \sigma^2)\Delta t + \sigma \sqrt{\Delta t}\, \varepsilon\right)
\]

Für tägliche Schritte mit annualisierten Parametern:

\[
\Delta t = \frac{1}{252}
\]

### Wie umsetzen?

Führe im Code explizit ein:

\[
dt = \frac{1}{252}
\]

und simuliere mit:

\[
S_j^{(d+1)} = S_j^{(d)} \exp\left((\mu_j - \tfrac12 \sigma_j^2)dt + \sigma_j \sqrt{dt}\,\varepsilon_j\right)
\]

Wenn \(\mu\) und \(\sigma\) bereits **daily** geschätzt werden, bleibt implizit \(dt = 1\) im Tagesraster.  
Trotzdem lohnt sich die explizite Modellierung, damit später keine Einheitenfehler entstehen.

---

## 4. Freiheitsgrade \(\nu\) robuster schätzen

### Warum?

Aktuell leitest du \(\nu\) aus der Excess Kurtosis ab:

\[
\kappa = \frac{6}{\nu - 4}
\quad \Longrightarrow \quad
\nu = 4 + \frac{6}{\kappa}
\]

Das ist mathematisch korrekt für eine Student-t-Verteilung, aber **statistisch instabil**:

- bei kleiner \(\kappa\) explodiert \(\nu\)
- bei verrauschter \(\kappa\) schwankt \(\nu\) stark
- nahe \(\nu = 4\) werden die Tails extrem

### Was ändern?

#### Variante A — Clamping

Setze

\[
\nu_i \in [\nu_{\min}, \nu_{\max}]
\]

z. B.

\[
\nu_{\min} = 5, \quad \nu_{\max} = 30
\]

Interpretation:

- unter 5 werden die Tails meist zu extrem
- ab 30 ist die Verteilung praktisch fast normal

#### Variante B — Gemeinsame Tail-Parameter für alle Assets

Statt für jedes Asset ein eigenes \(\nu_i\):

\[
\nu_1 = \cdots = \nu_n = \nu_{\text{global}}
\]

Das stabilisiert die Simulation und vermeidet inkonsistente Tail-Strukturen.

#### Variante C — Maximum-Likelihood statt Kurtosis-Inversion

Langfristig besser:

- passe eine Student-t-Verteilung per Likelihood an die Renditen an
- schätze \(\nu\) direkt numerisch

### Wie umsetzen?

Kurzfristig: Clamping beibehalten, zusätzlich ein oberes Cap.

Empfehlung:

\[
\nu_i \leftarrow \min(30, \max(5, \nu_i))
\]

---

## 5. Historische Volatilität durch EWMA ersetzen oder ergänzen

### Warum?

Deine historische Standardabweichung behandelt alle Tage gleich stark. Das ist für Märkte oft zu träge. Neuere Schocks sollten stärker gewichtet werden.

### Mathematisch

Ein klassischer Ansatz ist **EWMA** (Exponentially Weighted Moving Average):

\[
\sigma_t^2 = \lambda \sigma_{t-1}^2 + (1-\lambda) r_t^2
\]

mit typischem Wert

\[
\lambda = 0.94
\]

Das Modell reagiert schneller auf aktuelle Volatilitätsphasen.

### Was bringt das?

- aktuelleres Risikobild
- mehr Realismus nach Volatilitätsschocks
- weniger „zu ruhige“ Schätzung in stressigen Phasen

### Wie umsetzen?

Für jedes Asset:

1. initialisiere \(\sigma_0^2\) mit der Sample-Varianz
2. iteriere durch die Renditen
3. verwende den letzten EWMA-Wert als aktuelle Volatilität

Optional kannst du beides vergleichen:

- historische \(\sigma\)
- EWMA-\(\sigma\)

---

## 6. Korrelationen unter Stress erhöhen

### Warum?

Ein großes Problem vieler Portfoliomodelle:  
Historische Korrelationen aus normalen Marktphasen unterschätzen oft die Kopplung in Krisen.

In Stressphasen gilt häufig:

- Diversifikation wird schwächer
- Aktien fallen stärker gemeinsam
- linke Tails werden breiter

### Was ändern?

Baue einen **Stress-Korrelationsmodus** ein.

Zum Beispiel:

\[
\rho^{\text{stress}}_{ij} = \alpha \rho_{ij} + (1-\alpha)
\]

mit z. B.

\[
\alpha = 0.5
\]

Dann werden alle Off-Diagonal-Einträge näher an 1 gezogen.

Alternativ additiv konservativ:

\[
\rho^{\text{stress}}_{ij} = \min(1, \rho_{ij} + 0.2)
\]

für \(i \ne j\).

### Wie umsetzen?

Vor der Cholesky-Zerlegung:

1. normale Korrelationsmatrix berechnen
2. optional Stress-Transformation anwenden
3. danach Cholesky-Faktor berechnen

---

## 7. Multivariate Student-t statt „Assetweise t + Cholesky“

### Warum?

Aktuell erzeugst du für jedes Asset separat einen Student-t-Schock und korrelierst dann linear per Cholesky. Das ist ein brauchbarer Start, aber nicht vollständig konsistent mit einer echten **multivariaten Student-t-Struktur**.

### Mathematisch

Eine multivariate Student-t kann dargestellt werden als:

\[
\mathbf{X} = \frac{\mathbf{Y}}{\sqrt{U / \nu}}
\]

mit

- \(\mathbf{Y} \sim \mathcal{N}(0, \rho)\)
- \(U \sim \chi^2_\nu\)

Dann teilen sich alle Komponenten denselben Zufallsfaktor \(U\), wodurch gemeinsame Extremereignisse realistischer werden.

### Was bringt das?

- bessere gemeinsame Tail-Abhängigkeit
- realistischere Stressszenarien
- weniger „nur linear korreliert, aber nicht gemeinsam extrem“

### Wie umsetzen?

Statt assetweise unabhängige t-Stichproben:

1. simuliere \(\mathbf{Y} \sim N(0,\rho)\) via Cholesky
2. simuliere \(U \sim \chi^2_\nu\)
3. setze

\[
\boldsymbol{\varepsilon} = \mathbf{Y} \cdot \sqrt{\frac{\nu}{U}}
\]

Optional anschließend wieder passend skalieren.

Das wäre ein deutlicher Modell-Upgradeschritt.

---

## 8. Historical Bootstrapping als alternatives Simulationsmodell

### Warum?

GBM + Student-t ist immer noch ein Modell mit klaren Annahmen. Eine sehr starke Alternative ist **historisches Resampling**.

Dabei verwendest du echte historische Return-Vektoren statt parametrischer Verteilungen.

### Mathematisch

Sei

\[
\mathbf{r}^{(1)}, \ldots, \mathbf{r}^{(M)}
\]

die Menge historischer gemeinsamer Tagesrenditen aller Assets. Dann simulierst du für jeden Tag einen zufälligen historischen Tag:

\[
\mathbf{r}^{*(d)} \in \{\mathbf{r}^{(1)}, \ldots, \mathbf{r}^{(M)}\}
\]

und setzt

\[
S_j^{(d+1)} = S_j^{(d)} \exp(r_j^{*(d)})
\]

### Was bringt das?

- echte historische Tails
- echte historische Korrelationen
- keine separate Schätzung von \(\nu\) nötig

### Nachteile

- nur historische Muster werden wiederverwendet
- keine neuen, „noch nie gesehenen“ Extremwerte

### Wie umsetzen?

Zusätzlichen Simulationsmodus einbauen:

- `parametric_gbm_t`
- `historical_bootstrap`

Optional blockweise bootstrappen, damit Volatilitätscluster teilweise erhalten bleiben.

---

## 9. Block-Bootstrapping für Regime und Volatilitätscluster

### Warum?

Einzelnes Resampling von Tagen zerstört zeitliche Abhängigkeiten wie:

- Volatilitätscluster
- mehrtägige Drawdowns
- Trend-/Panikphasen

### Was ändern?

Resample nicht einzelne Tage, sondern Blöcke von Länge \(B\):

\[
(\mathbf{r}^{(k)}, \mathbf{r}^{(k+1)}, \ldots, \mathbf{r}^{(k+B-1)})
\]

### Was bringt das?

- bessere Erhaltung lokaler Dynamik
- realistischere Drawdowns
- bessere Mehrtagesrisiken

### Wie umsetzen?

Wähle z. B.

\[
B = 5 \text{ bis } 20
\]

und füge zufällige Blöcke aneinander, bis `horizon_days` erreicht ist.

---

## 10. Regime-Switching einführen

### Warum?

Ein einziges \((\mu,\sigma,\rho)\)-Regime ist oft zu simpel. Märkte wechseln zwischen:

- ruhigen Regimen
- volatilen Regimen
- Crash-Regimen

### Mathematisch

Definiere z. B. zwei Regime:

- Regime 1: niedriges \(\sigma\), normale Korrelation
- Regime 2: hohes \(\sigma\), erhöhte Korrelation

Dann wird ein Zustandsprozess simuliert, etwa über eine Markov-Kette:

\[
P(X_{t+1}=j \mid X_t=i) = p_{ij}
\]

und die Parameter hängen vom Regime ab.

### Was bringt das?

- realistischere Volatilitätscluster
- plötzliche Stressphasen
- weniger „zu glatte“ Simulationen

### Wie umsetzen?

Einfacher Start:

- 2 Regime definieren
- feste Übergangswahrscheinlichkeiten wählen
- pro Tag je nach Regime unterschiedliche \(\sigma\)- und \(\rho\)-Parameter verwenden

---

## 11. Portfolio nicht nur über Endwert, sondern auch über Pfadrisiken auswerten

### Warum?

Aktuell misst du nur den **finalen Wert**. Das ist für viele Fragestellungen zu wenig. Ein Portfolio kann am Ende okay aussehen, zwischendurch aber einen massiven Drawdown gehabt haben.

### Zusätzliche Metriken

#### Maximaler Drawdown

Für einen simulierten Pfad \(V^{(0)}, \ldots, V^{(T)}\):

\[
\text{MDD} = \max_t \left(1 - \frac{V_t}{\max_{s \le t} V_s}\right)
\]

#### Time under Water

Anzahl Tage unter dem bisherigen Hoch.

#### Probability of Loss

\[
P(V_T < V_0)
\]

#### Probability of Large Loss

\[
P(V_T < 0.8 V_0)
\]

### Wie umsetzen?

Während jeder Simulation nicht nur den Endwert speichern, sondern optional auch den Verlauf des Portfolio-Werts pro Tag berechnen.

---

## 12. Histogramm und Quantile auf Returns statt auf absoluten Werten

### Warum?

Absolute Endwerte sind schwerer zu interpretieren. Returns machen Ergebnisse direkt vergleichbar zwischen Portfolios.

### Mathematisch

Definiere den simulierten Portfolio-Return als:

\[
R = \frac{V_T}{V_0} - 1
\]

Dann kannst du Histogramme, VaR und Wahrscheinlichkeiten direkt auf Return-Basis darstellen.

### Was bringt das?

- bessere Interpretierbarkeit
- leichtere Vergleiche zwischen Simulationen
- natürlicher für Risikodarstellung

### Wie umsetzen?

Zusätzlich zu `final_portfolio_values`:

\[
R_k = \frac{V_k}{V_0} - 1
\]

sortieren und auswerten.

---

## 13. Quantilschätzung verbessern

### Warum?

Aktuell verwendest du für VaR/CVaR einen diskreten Index:

\[
\lfloor \alpha N \rfloor
\]

Das ist okay, aber bei kleineren Stichproben etwas grob.

### Verbesserung

Nutze lineare Interpolation zwischen benachbarten Ordnungstatistiken.

Wenn

\[
q = \alpha (N-1)
\]

mit

\[
k = \lfloor q \rfloor, \quad \delta = q-k
\]

dann:

\[
V_{\alpha} \approx (1-\delta)V_{(k)} + \delta V_{(k+1)}
\]

### Wie umsetzen?

Für größere `n_sims` kein Muss, aber mathematisch sauberer.

---

## 14. Mehr Simulationen für stabile Tail-Schätzung

### Warum?

Tail-Risikomaße wie 99%-VaR und 99%-CVaR sind empfindlich auf Simulationsrauschen.

Bei

\[
N_{\text{sims}} = 10{,}000
\]

liegen im 1%-Tail nur etwa 100 Beobachtungen. Das ist wenig für stabile CVaR-Schätzung.

### Was ändern?

Erhöhe auf:

\[
100{,}000 \text{ bis } 1{,}000{,}000
\]

je nach Rechenzeit.

### Wie umsetzen?

Deine Parallelisierung ist dafür schon die richtige Grundlage.  
Zusätzlich kannst du:

- `std::nth_element` statt vollständigem `sort` für reine Quantile nutzen
- separate Modi für „schneller Überblick“ und „genaue Risikoanalyse“ einführen

---

## 15. Szenario- und Stresstests ergänzen

### Warum?

Monte Carlo ist nur eine Sicht auf Risiko. Gute Risikoanalyse ergänzt feste Schockszenarien.

### Beispiele

#### Sofortiger Ein-Tages-Schock

- Tech-Crash \(-15\%\)
- Volatilitätssprung
- Korrelationsanstieg

#### Mehrtägiger Stresspfad

- 5 Tage negative Returns
- anschließend hohe Volatilität

### Wie umsetzen?

Zusätzlichen Modus bauen, in dem deterministisch oder halbdeterministisch Renditepfade vorgegeben werden:

\[
\mathbf{r}^{(1)}, \ldots, \mathbf{r}^{(T)}
\]

Dann dieselbe Portfoliologik auf diese Pfade anwenden.

---

# Empfohlene Reihenfolge

Wenn du die Verbesserungen nicht alle gleichzeitig umsetzen willst, ist diese Reihenfolge sinnvoll:

## Phase 1 — Sofortige Qualitätsverbesserung

1. Drift entschärfen (`mu = 0` oder Shrinkage)
2. längere Historie nutzen
3. \(\nu\) zusätzlich nach oben und unten cappen
4. Returns statt absolute Endwerte visualisieren
5. mehr Simulationen für stabilere Tails

## Phase 2 — Mehr Realismus

6. EWMA-Volatilität
7. Stress-Korrelationen
8. Pfadrisiken wie Drawdown ergänzen
9. Szenario-Stresstests hinzufügen

## Phase 3 — Modell-Upgrades

10. Historical Bootstrapping
11. Block-Bootstrapping
12. Multivariate Student-t
13. Regime-Switching

---

# Kurzfazit

Dein aktueller Algorithmus ist bereits ein solider parametrisierter Monte-Carlo-Rahmen. Die wichtigsten nächsten Verbesserungen zielen nicht auf „mehr Mathematik“, sondern auf **robustere Parameter, realistischere Tails und bessere Stressmodellierung**.

Die drei wirkungsvollsten Upgrades sind wahrscheinlich:

1. **Drift konservativer behandeln**
2. **längere und robustere Datenbasis nutzen**
3. **historisches oder blockweises Bootstrapping als Vergleichsmodell ergänzen**

Wenn du möchtest, kann ich als Nächstes noch ein **zweites Markdown-Dokument speziell als technische Roadmap für die Implementierung in C++** schreiben — also in welcher Reihenfolge du Klassen, Funktionen und Datenstrukturen erweitern solltest.
