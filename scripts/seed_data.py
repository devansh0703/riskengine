#!/usr/bin/env python3
"""Generate sample trade and price data for RiskEngine."""
import csv
import random
import sys
import os

random.seed(42)

SYMBOLS = ["AAPL", "GOOG", "MSFT", "AMZN", "TSLA", "NVDA", "META", "JPM",
           "V", "WMT", "UNH", "MA", "JNJ", "PG", "HD", "BAC", "XOM", "PFE",
           "ABBV", "KO"]
STRATEGIES = ["stat_arb", "momentum", "market_making", "vol_arb", "macro"]
ACCOUNTS = ["acc1", "acc2", "acc3"]

BASE_PRICES = {
    "AAPL": 175, "GOOG": 140, "MSFT": 380, "AMZN": 180, "TSLA": 250,
    "NVDA": 450, "META": 350, "JPM": 180, "V": 270, "WMT": 165,
    "UNH": 520, "MA": 420, "JNJ": 155, "PG": 155, "HD": 350,
    "BAC": 35, "XOM": 105, "PFE": 28, "ABBV": 155, "KO": 60,
}


def generate_trades(output_path: str, n: int = 10000):
    with open(output_path, "w", newline="") as f:
        writer = csv.writer(f)
        writer.writerow(["trade_id", "timestamp", "symbol", "side", "quantity",
                          "price", "fees", "strategy", "account", "fill_id"])

        ts = 1700000000000000000  # Nov 2023 in ns
        for i in range(n):
            symbol = random.choice(SYMBOLS)
            side = random.choice(["Buy", "Sell"])
            qty = random.randint(1, 1000) * 10
            base = BASE_PRICES[symbol]
            price = round(base * (1 + random.gauss(0, 0.02)), 2)
            fees = round(qty * price * 0.0001, 2)
            strategy = random.choice(STRATEGIES)
            account = random.choice(ACCOUNTS)
            fill_id = f"fill_{i:08d}"

            writer.writerow([i, ts, symbol, side, qty, price, fees,
                             strategy, account, fill_id])
            ts += random.randint(100000, 5000000)

    print(f"Generated {n} trades -> {output_path}")


def generate_prices(output_path: str, n: int = 100000):
    with open(output_path, "w", newline="") as f:
        writer = csv.writer(f)
        writer.writerow(["timestamp", "symbol", "bid", "ask", "last"])

        ts = 1700000000000000000
        prices = {s: BASE_PRICES[s] for s in SYMBOLS}

        for i in range(n):
            symbol = random.choice(SYMBOLS)
            p = prices[symbol]
            change = random.gauss(0, 0.002)
            p = round(p * (1 + change), 2)
            prices[symbol] = p

            spread = round(p * 0.001, 2)
            bid = round(p - spread / 2, 2)
            ask = round(p + spread / 2, 2)

            writer.writerow([ts, symbol, bid, ask, p])
            ts += random.randint(100000, 1000000)

    print(f"Generated {n} prices -> {output_path}")


if __name__ == "__main__":
    data_dir = os.path.join(os.path.dirname(__file__), "..", "data")
    os.makedirs(data_dir, exist_ok=True)

    n_trades = int(sys.argv[1]) if len(sys.argv) > 1 else 10000
    n_prices = int(sys.argv[2]) if len(sys.argv) > 2 else 100000

    generate_trades(os.path.join(data_dir, "sample_trades.csv"), n_trades)
    generate_prices(os.path.join(data_dir, "sample_prices.csv"), n_prices)
