"""Historical replay engine."""
import csv
import time
from dataclasses import dataclass
from risk.client import RiskEngineClient


@dataclass
class ReplayConfig:
    trades_path: str = ""
    prices_path: str = ""
    speed: float = 1.0  # multiplier: 1.0 = realtime, 0 = instant
    start_ns: int = 0
    end_ns: int = 0


class Replay:
    """Replay historical trades and prices through the engine."""

    def __init__(self, config: ReplayConfig):
        self.config = config
        self.client = RiskEngineClient()
        self._trades: list[dict] = []
        self._prices: list[dict] = []
        self._replayed = 0

    def load(self):
        if self.config.trades_path:
            with open(self.config.trades_path) as f:
                self._trades = list(csv.DictReader(f))
            self._trades.sort(key=lambda r: int(r["timestamp"]))

        if self.config.prices_path:
            with open(self.config.prices_path) as f:
                self._prices = list(csv.DictReader(f))
            self._prices.sort(key=lambda r: int(r["timestamp"]))

    def run(self):
        from risk.client import Trade

        self.load()
        print(f"Loaded {len(self._trades)} trades, {len(self._prices)} prices")

        price_idx = 0
        last_ts = 0

        for i, row in enumerate(self._trades):
            ts = int(row["timestamp"])

            while price_idx < len(self._prices):
                p = self._prices[price_idx]
                if int(p["timestamp"]) > ts:
                    break
                self.client._positions.setdefault(
                    p["symbol"],
                    type("Pos", (), {"current_price": 0, "symbol": p["symbol"]})(),
                )
                price_idx += 1

            trade = Trade(
                trade_id=int(row["trade_id"]),
                timestamp_ns=ts,
                symbol=row["symbol"],
                side=row["side"],
                quantity=int(row["quantity"]),
                price=float(row["price"]),
                fees=float(row["fees"]),
                strategy=row.get("strategy", ""),
                account=row.get("account", ""),
                fill_id=row.get("fill_id", ""),
            )
            self.client.ingest_trade(trade)
            self._replayed += 1

            if self.config.speed > 0 and last_ts > 0:
                delta = ts - last_ts
                if delta > 0:
                    time.sleep(delta * 1e-9 * self.config.speed)

            last_ts = ts

            if (i + 1) % 100 == 0:
                print(f"  Replayed {i + 1}/{len(self._trades)} trades")

        pnl = self.client.get_pnl()
        print(f"\nReplay complete: {self._replayed} trades")
        print(f"Realized: ${pnl.realized:,.2f}")
        print(f"Unrealized: ${pnl.unrealized:,.2f}")
        print(f"Total: ${pnl.total:,.2f}")
