# API Endpoint Tests (curl)

Basis-URL: `http://localhost:8080`  
Anpassen falls Server auf anderem Port läuft.

---

## GET /health

Prüft ob der Server läuft.

```bash
curl -s http://localhost:8080/health
```

Erwartete Antwort:
```json
{"status":"ok"}
```

---

## POST /asset

Gibt Kennzahlen für ein einzelnes Asset zurück (Volatilität, Sharpe Ratio, Max Drawdown, etc.).

**Aktie:**
```bash
curl -s -X POST http://localhost:8080/asset \
  -H "Content-Type: application/json" \
  -d '{
    "ticker": "AAPL",
    "type": "stock"
  }'
```

**Krypto:**
```bash
curl -s -X POST http://localhost:8080/asset \
  -H "Content-Type: application/json" \
  -d '{
    "ticker": "bitcoin",
    "type": "crypto"
  }'
```

---

## POST /simulate

Führt eine Monte-Carlo-Simulation für ein Portfolio durch.

**Einfaches 2-Asset Portfolio (Aktien):**
```bash
curl -s -X POST http://localhost:8080/simulate \
  -H "Content-Type: application/json" \
  -d '{
    "portfolio_value": 100000,
    "horizon_days": 252,
    "n_simulations": 10000,
    "assets": [
      { "ticker": "AAPL", "type": "stock", "weight": 0.6 },
      { "ticker": "MSFT", "type": "stock", "weight": 0.4 }
    ]
  }'
```

**Gemischtes Portfolio (Aktie + Krypto):**
```bash
curl -s -X POST http://localhost:8080/simulate \
  -H "Content-Type: application/json" \
  -d '{
    "portfolio_value": 50000,
    "horizon_days": 90,
    "n_simulations": 5000,
    "assets": [
      { "ticker": "AAPL",    "type": "stock",  "weight": 0.5 },
      { "ticker": "bitcoin", "type": "crypto", "weight": 0.3 },
      { "ticker": "TSLA",    "type": "stock",  "weight": 0.2 }
    ]
  }'
```

Relevante Felder in der Antwort: `var_95`, `var_99`, `cvar_95`, `cvar_99`, `avg`, `min`, `max`, `bin_width`, `histogram_bins`

---

## POST /correlation

Berechnet die Korrelationsmatrix für mehrere Assets.

```bash
curl -s -X POST http://localhost:8080/correlation \
  -H "Content-Type: application/json" \
  -d '{
    "assets": [
      { "ticker": "AAPL",    "type": "stock"  },
      { "ticker": "MSFT",    "type": "stock"  },
      { "ticker": "bitcoin", "type": "crypto" }
    ]
  }'
```

Die Antwort enthält die Korrelationen als verschachteltes JSON-Objekt, z.B. `response["AAPL"]["MSFT"]`.

---

## Fehlerfall testen

Ungültiger Body → sollte `400` mit `error`-Feld zurückgeben:

```bash
curl -s -o /dev/null -w "%{http_code}" -X POST http://localhost:8080/simulate \
  -H "Content-Type: application/json" \
  -d '{"invalid": true}'
```

---

## Ausgabe formatieren (mit `jq`)

```bash
curl -s -X POST http://localhost:8080/asset \
  -H "Content-Type: application/json" \
  -d '{"ticker": "AAPL", "type": "stock"}' | jq .
```
