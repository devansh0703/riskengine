"""Trade Log panel."""
from dash import dash_table, html, dcc
import pandas as pd
from risk.client import RiskEngineClient


def render_trade_log(client: RiskEngineClient) -> html.Div:
    positions = client.get_positions()
    if not positions:
        return html.Div("No trades", style={
            "textAlign": "center", "padding": "40px", "color": "#888"
        })

    rows = []
    for p in positions:
        rows.append({
            "Symbol": p.symbol,
            "Net Qty": f"{p.net_quantity:,}",
            "Avg Entry": f"${p.avg_entry_price:,.2f}",
            "Realized PnL": f"${p.realized_pnl:,.2f}",
            "Unrealized PnL": f"${p.unrealized_pnl:,.2f}",
            "Trades": str(p.trade_count),
        })

    df = pd.DataFrame(rows)

    return html.Div([
        html.H3("Trade Log", style={"color": "#00d4ff", "marginBottom": "10px"}),
        html.Div(style={
            "display": "flex", "gap": "10px", "marginBottom": "10px",
        }, children=[
            dcc.Input(
                id="trade-filter-symbol",
                placeholder="Filter by symbol...",
                style={
                    "backgroundColor": "#16213e", "color": "#e0e0e0",
                    "border": "1px solid #333", "padding": "8px",
                    "borderRadius": "3px", "width": "200px",
                },
            ),
        ]),
        dash_table.DataTable(
            id="trade-log-table",
            data=df.to_dict("records"),
            columns=[{"name": c, "id": c} for c in df.columns],
            sort_action="native",
            page_size=25,
            style_table={"overflowX": "auto"},
            style_header={
                "backgroundColor": "#0f3460",
                "color": "white",
                "fontWeight": "bold",
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
                {
                    "if": {"filter_query": '{Net Qty} contains "-"'},
                    "backgroundColor": "rgba(255,68,68,0.05)",
                },
            ],
        ),
    ])
