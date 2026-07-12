# RiskEngine

Real-time risk management and PnL tracking system implemented in C++20 with a Python Plotly Dash dashboard. Position tracking with concurrent updates, FIFO/LIFO/average-cost lot matching, VaR computation, Black-Scholes Greeks, and a rule-based alert engine with state machine workflow.

## Overview

RiskEngine is the risk management layer for a trading system. It tracks positions across symbols, strategies, and accounts with sub-microsecond update latency. PnL is computed in real-time using three lot matching methods (FIFO, LIFO, average cost). The metrics engine calculates Value at Risk via historical simulation, parametric, and Monte Carlo methods, and computes Greeks (Delta, Gamma, Vega, Theta, Rho) analytically using Black-Scholes.

A rule-based alert engine monitors positions, PnL, and exposure against configurable limits, with a state machine workflow (Pending, Active, Acknowledged, Resolved, Escalated, WrittenOff). Trade history is persisted in SQLite with WAL mode for concurrent reads during writes.

The Python dashboard provides live visualization of PnL curves, position tables, risk limit utilization, exposure heatmaps, and VaR displays.

## Architecture

```
Trade Ingestion (Live WebSocket / CSV Replay)
        |
    +---+------------------+
    | PositionTracker      |  Concurrent position updates, per-symbol/strategy/account
    +---+------------------+
        |
    +---+------------------+
    | PnLCalculator        |  Realized (lot matching) + unrealized (mark-to-market)
    +---+------------------+
        |
    +---+------------------+
    | LotMatching          |  FIFO, LIFO, AverageCost
    +---+------------------+
        |
    +---+------------------+
    | MetricsEngine        |  VaR, Greeks, exposure, position/loss limits
    +---+------------------+
        |
    +---+------------------+
    | AlertManager         |  Rule evaluation, state machine, notification channels
    +---+------------------+
        |
    +---+------------------+
    | TradeJournal         |  SQLite WAL, append-only, indexed queries
    +----------------------+
```

## Features

### Position Tracker

- Tracks net position per symbol, per strategy, per account
- Thread-safe concurrent updates via `std::mutex`
- Realized PnL computed on each trade via lot matching
- Unrealized PnL via mark-to-market with latest market data price
- Position history: timestamped snapshots for audit trail
- Sub-microsecond position update latency

### PnL Calculator

- Realized PnL: (exit_price - entry_price) x quantity x direction
- Unrealized PnL: (current_price - avg_entry) x net_position
- Total PnL: realized + unrealized - fees - commissions
- Daily PnL reset at configurable cutoff time
- Cumulative PnL: from account inception, rolling 30-day, rolling 90-day
- Attribution: PnL by symbol, by strategy, by side, by hourly time bucket

### Lot Matching

Three methods, each producing correct realized PnL:

| Method | Logic |
|--------|-------|
| FIFO | Match against oldest lots first |
| LIFO | Match against newest lots first |
| AverageCost | Weighted average entry price across all lots |

Each method tracks remaining lots after every trade and computes realized PnL per fill.

### Risk Metrics Engine

| Metric | Calculation |
|--------|-------------|
| Position Limits | Per-symbol, per-strategy, per-account (hard and soft) |
| Loss Limits | Daily stop-loss, max drawdown, drawdown duration |
| Exposure | Gross/net exposure, HHI concentration, sector exposure |
| VaR (Historical) | Simulation from historical return distribution |
| VaR (Parametric) | Normal distribution assumption |
| VaR (Monte Carlo) | Random scenario generation |
| Greeks | Black-Scholes analytical: Delta, Gamma, Vega, Theta, Rho |
| Margin | Initial margin, maintenance margin (simplified SIMM) |

### Greeks (Black-Scholes)

Analytical computation for European options:

- Delta: dV/dS (call and put)
- Gamma: d2V/dS2
- Vega: dV/dsigma
- Theta: -dV/dT (call and put)
- Rho: dV/dr (call and put)
- Implied volatility: Newton-Raphson solver from market price

### Alert System

Rule-based engine with state machine workflow:

```
Detected -> Active -> Acknowledged -> Resolved
                                  -> Escalated
                                  -> WrittenOff
```

| Rule | Condition | Severity |
|------|-----------|----------|
| Position limit | Exceeds max long/short | Critical |
| Daily stop-loss | PnL hits threshold | Critical |
| Drawdown | Exceeds max drawdown | Warning |
| VaR breach | Position VaR exceeds limit | Warning |
| Market data staleness | Price older than threshold | Info |

Notification channels: console log, file, webhook (Slack/PagerDuty).

### Trade Journal

- SQLite with WAL mode for concurrent reads during writes
- Append-only immutable log
- Indexed queries: by time range, symbol, strategy, account
- Export to CSV/Parquet for analytical queries
- Deduplication on trade_id

### Market Data Handler

- WebSocket client for live price feeds
- Price cache: latest bid/ask/last per symbol, updated in nanoseconds
- Staleness detection: flags prices older than configurable threshold
- Market hours detection per exchange
- Replay mode: read prices from CSV with simulated timing

## Dashboard

Python Plotly Dash application with six panels:

| Panel | Content |
|-------|---------|
| PnL Curve | Realized + unrealized PnL over time with drawdown overlay |
| Position Table | Live positions with unrealized PnL, updated in real-time |
| Risk Limits | Current utilization vs limits (gauge charts) |
| Exposure Heatmap | Concentration by symbol, by strategy |
| VaR Display | Current VaR, historical VaR backtest |
| Trade Log | Scrollable table of recent trades with filtering |

Launch with: `python -m risk.dashboard.app` (port 8050)

## CLI

```bash
risk-engine serve    --port 9090 --market-data ws://localhost:8080
risk-engine status                           # Current positions, PnL, limits
risk-engine journal  --from 2025-01-01 --to 2025-06-01 --symbol BTCUSDT
risk-engine export   --format parquet --output today_pnl.parquet
risk-engine replay   --file trades.csv --dashboard
risk-engine test-limits                      # Verify all limit configurations
risk-engine dashboard                        # Launch Plotly Dash UI
```

## Getting Started

### Prerequisites

- C++20 compiler (GCC 12+ or Clang 15+)
- CMake 3.20+
- SQLite3
- Python 3.10+ (for dashboard)
- pybind11 (for Python bindings)

### Build

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=ON -DBUILD_TOOLS=ON
make -j$(nproc)
ctest --output-on-failure
```

### Generate Sample Data

```bash
python3 scripts/seed_data.py
# Creates data/sample_trades.csv (10k trades) and data/sample_prices.csv (100k ticks)
```

### Dashboard

```bash
pip install -e ".[dev]"
python -m risk.dashboard.app
# Opens at http://localhost:8050
```

### Docker

```bash
docker-compose up --build
# Includes risk engine, dashboard, and demo data
```

## Configuration

| File | Purpose |
|------|---------|
| `config/default.yaml` | Risk limits, alert thresholds, auto-resolution rules |
| `config/strategies.yaml` | Strategy definitions and parameters |
| `config/market_hours.yaml` | Market hours by exchange |

## Tech Stack

| Layer | Technology |
|-------|------------|
| Core Engine | C++20, std::mutex for thread safety |
| Risk Models | Black-Scholes (Greeks), Historical/Parametric/Monte Carlo (VaR) |
| Storage | SQLite (WAL mode), CSV export |
| Market Data | WebSocket (live), file replay |
| Dashboard | Python, Plotly Dash, WebSocket client |
| Bindings | pybind11 |
| CLI | Python (Click), C++ (native) |
| Testing | Google Test (C++), pytest (Python) |
| Build | CMake, pyproject.toml |

## Testing

C++ test suite (10 suites):

- Position tracker: single trade, multiple trades, concurrent updates
- PnL calculator: long/short, realized/unrealized, fees, daily reset
- Lot matching: FIFO/LIFO/average-cost with known inputs, edge cases
- Metrics engine: VaR verification, Greeks vs Black-Scholes reference
- Risk limits: within/at/over limits, soft vs hard
- Alert manager: all state transitions, timeout handling
- Trade journal: SQLite logging, query, dedup
- Market data: feed parsing, stale data, gap recovery
- Stress test: 4 threads ingesting trades, ThreadSanitizer
- Greeks: analytical values vs reference implementation

Python test suite (3 suites): position tracking, PnL calculation, dashboard rendering.

## Project Structure

```
riskengine/
  include/risk/       Headers: Position, PositionTracker, PnLCalculator, LotMatching,
                       MetricsEngine, RiskLimits, AlertManager, TradeJournal, MarketData
  src/                Implementations (~3000 lines C++)
  greeks/             Black-Scholes pricing, implied vol solver, Greeks tests
  python/             pybind11 bindings, dashboard (Plotly Dash), client, replay
  test/               Google Test (10 suites)
  tools/              risk_cli, replay_tool, export_tool
  config/             YAML: risk limits, strategies, market hours
  data/               Sample trades (10k), sample prices (100k)
  scripts/            Build, test, Python setup, data seeding
  .github/workflows/  CI pipeline
```
