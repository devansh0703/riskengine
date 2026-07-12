"""WebSocket client for live risk updates."""
import json
import asyncio
from typing import Callable, Optional
from dataclasses import dataclass


@dataclass
class RiskUpdate:
    symbol: str
    net_quantity: int
    current_price: float
    unrealized_pnl: float
    realized_pnl: float
    timestamp_ns: int


class RiskWebSocket:
    """WebSocket client for receiving live updates from the C++ engine."""

    def __init__(self, url: str = "ws://localhost:9000"):
        self.url = url
        self._callbacks: list[Callable] = []
        self._running = False
        self._ws = None

    def on_update(self, callback: Callable):
        self._callbacks.append(callback)

    async def connect(self):
        try:
            import websockets
            self._running = True
            async with websockets.connect(self.url) as ws:
                self._ws = ws
                while self._running:
                    try:
                        msg = await asyncio.wait_for(ws.recv(), timeout=5.0)
                        data = json.loads(msg)
                        update = RiskUpdate(
                            symbol=data.get("symbol", ""),
                            net_quantity=data.get("net_quantity", 0),
                            current_price=data.get("current_price", 0),
                            unrealized_pnl=data.get("unrealized_pnl", 0),
                            realized_pnl=data.get("realized_pnl", 0),
                            timestamp_ns=data.get("timestamp_ns", 0),
                        )
                        for cb in self._callbacks:
                            cb(update)
                    except asyncio.TimeoutError:
                        continue
                    except Exception:
                        break
        except ImportError:
            pass
        except Exception:
            pass

    def disconnect(self):
        self._running = False

    def start(self):
        asyncio.run(self.connect())
