"""Positions table panel."""
from dash import dash_table, html
import pandas as pd
from risk.client import RiskEngineClient


def render_positions(client: RiskEngineClient) -> html.Div:
    positions = client.get_positions()
    if not positions:
        return html.Div("No open positions", style={
            "textAlign": "center", "padding": "40px", "color": "#888"
        })

    rows = []
    for p in sorted(positions, key=lambda x: abs(x.net_quantity), reverse=True):
        total_pnl = p.realized_pnl + p.unrealized_pnl
        rows.append({
            "Symbol": p.symbol,
            "Side": "LONG" if p.net_quantity > 0 else "SHORT",
            "Quantity": f"{abs(p.net_quantity):,}",
            "Avg Entry": f"${p.avg_entry_price:,.2f}",
            "Current Price": f"${p.current_price:,.2f}",
            "Realized PnL": f"${p.realized_pnl:,.2f}",
            "Unrealized PnL": f"${p.unrealized_pnl:,.2f}",
            "Total PnL": f"${total_pnl:,.2f}",
            "Trades": str(p.trade_count),
        })

    df = pd.DataFrame(rows)

    return html.Div([
        html.H3("Open Positions", style={"color": "#00d4ff", "marginBottom": "10px"}),
        dash_table.DataTable(
            data=df.to_dict("records"),
            columns=[{"name": c, "id": c} for c in df.columns],
            sort_action="native",
            filter_action="native",
            page_size=20,
            style_table={"overflowX": "auto"},
            style_header={
                "backgroundColor": "#0f3460",
                "color": "white",
                "fontWeight": "bold",
                "textAlign": "center",
            },
            style_cell={
                "backgroundColor": "#1a1a2e",
                "color": "#e0e0e0",
                "border": "1px solid #333",
                "textAlign": "center",
                "fontSize": "13px",
                "padding": "6px",
            },
            style_data_conditional=[
                {"if": {"filter_query": '{Side} = "LONG"'},
                 "backgroundColor": "rgba(0,255,136,0.05)"},
                {"if": {"filter_query": '{Side} = "SHORT"'},
                 "backgroundColor": "rgba(255,68,68,0.05)"},
            ],
        ),
    ])
