"""RiskEngine Python client."""
import json
import click
import time
from dataclasses import dataclass, field
from typing import Optional, Callable


@dataclass
class Trade:
    trade_id: int
    timestamp_ns: int
    symbol: str
    side: str
    quantity: int
    price: float
    fees: float
    strategy: str = ""
    account: str = ""
    fill_id: str = ""


@dataclass
class Position:
    symbol: str
    net_quantity: int
    avg_entry_price: float
    realized_pnl: float
    unrealized_pnl: float
    current_price: float
    trade_count: int = 0


@dataclass
class PnLSnapshot:
    realized: float = 0.0
    unrealized: float = 0.0
    total: float = 0.0
    fees: float = 0.0
    max_drawdown: float = 0.0


class RiskEngineClient:
    """Client to interact with RiskEngine."""

    def __init__(self, host: str = "localhost", port: int = 9000):
        self.host = host
        self.port = port
        self._positions: dict[str, Position] = {}
        self._pnl = PnLSnapshot()
        self._callbacks: list[Callable] = []

    def connect(self):
        pass

    def disconnect(self):
        pass

    def get_positions(self) -> list[Position]:
        return list(self._positions.values())

    def get_position(self, symbol: str) -> Optional[Position]:
        return self._positions.get(symbol)

    def get_pnl(self) -> PnLSnapshot:
        return self._pnl

    def on_update(self, callback: Callable):
        self._callbacks.append(callback)

    def ingest_trade(self, trade: Trade):
        pos = self._positions.get(trade.symbol)
        sign = 1 if trade.side == "Buy" else -1
        new_qty = sign * trade.quantity

        if pos is None:
            pos = Position(
                symbol=trade.symbol,
                net_quantity=new_qty,
                avg_entry_price=trade.price,
                realized_pnl=0.0,
                unrealized_pnl=0.0,
                current_price=trade.price,
                trade_count=0,
            )
        else:
            old_net = pos.net_quantity
            pos.net_quantity += new_qty
            pos.trade_count += 1

            close_qty = min(abs(new_qty), abs(old_net))
            if close_qty > 0:
                direction = 1 if old_net > 0 else -1
                pos.realized_pnl += (
                    direction * close_qty * (trade.price - pos.avg_entry_price)
                )

            if pos.net_quantity == 0:
                pos.avg_entry_price = 0.0
            elif old_net == 0:
                pos.avg_entry_price = trade.price
            elif (old_net > 0 and new_qty > 0) or (
                old_net < 0 and new_qty < 0
            ):
                old_cost = pos.avg_entry_price * abs(old_net)
                new_cost = trade.price * abs(new_qty)
                pos.avg_entry_price = (old_cost + new_cost) / abs(
                    pos.net_quantity
                )

            pos.current_price = trade.price

        self._positions[trade.symbol] = pos
        self._pnl.fees += trade.fees
        self._update_pnl()
        for cb in self._callbacks:
            cb(pos)

    def _update_pnl(self):
        total_unrealized = 0.0
        for pos in self._positions.values():
            if pos.net_quantity != 0:
                sign = 1 if pos.net_quantity > 0 else -1
                pos.unrealized_pnl = (
                    sign * abs(pos.net_quantity) * (pos.current_price - pos.avg_entry_price)
                )
            else:
                pos.unrealized_pnl = 0.0
            total_unrealized += pos.unrealized_pnl

        total_realized = sum(p.realized_pnl for p in self._positions.values())
        self._pnl.realized = total_realized
        self._pnl.unrealized = total_unrealized
        self._pnl.total = total_realized + total_unrealized - self._pnl.fees

    def load_trades_csv(self, path: str):
        import csv
        with open(path) as f:
            reader = csv.DictReader(f)
            for row in reader:
                trade = Trade(
                    trade_id=int(row["trade_id"]),
                    timestamp_ns=int(row["timestamp"]),
                    symbol=row["symbol"],
                    side=row["side"],
                    quantity=int(row["quantity"]),
                    price=float(row["price"]),
                    fees=float(row["fees"]),
                    strategy=row.get("strategy", ""),
                    account=row.get("account", ""),
                    fill_id=row.get("fill_id", ""),
                )
                self.ingest_trade(trade)


@click.group()
def cli():
    """RiskEngine CLI client."""
    pass


@cli.command()
@click.option("--csv", "csv_path", required=True, help="Trades CSV path")
def replay(csv_path: str):
    """Replay trades from CSV file."""
    client = RiskEngineClient()
    client.load_trades_csv(csv_path)

    positions = client.get_positions()
    click.echo(f"Replayed {len(positions)} unique symbols")
    pnl = client.get_pnl()
    click.echo(f"Total PnL: ${pnl.total:,.2f}")
    click.echo(f"Realized: ${pnl.realized:,.2f}")
    click.echo(f"Unrealized: ${pnl.unrealized:,.2f}")
    click.echo(f"Fees: ${pnl.fees:,.2f}")


@cli.command()
def status():
    """Show current engine status."""
    click.echo("RiskEngine status: not connected")


@cli.command()
@click.option("--port", default=8050, help="Dashboard port")
def dashboard(port: int):
    """Launch Plotly Dash dashboard."""
    from risk.dashboard.app import create_app
    app = create_app()
    app.run(debug=False, port=port, host="0.0.0.0")


if __name__ == "__main__":
    cli()
