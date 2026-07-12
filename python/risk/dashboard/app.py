"""Plotly Dash dashboard for RiskEngine."""
import dash
from dash import dcc, html, dash_table
from dash.dependencies import Input, Output
import plotly.graph_objects as go
import plotly.express as px
from plotly.subplots import make_subplots
import pandas as pd
import numpy as np
from risk.client import RiskEngineClient


_client: RiskEngineClient | None = None


def get_client() -> RiskEngineClient:
    global _client
    if _client is None:
        _client = RiskEngineClient()
    return _client


def create_app() -> dash.Dash:
    app = dash.Dash(
        __name__,
        title="RiskEngine Dashboard",
        update_title=None,
        suppress_callback_exceptions=True,
    )

    app.layout = html.Div(
        style={"fontFamily": "monospace", "backgroundColor": "#1a1a2e",
               "color": "#e0e0e0", "padding": "10px"},
        children=[
            html.H1(
                "RiskEngine Dashboard",
                style={"textAlign": "center", "color": "#00d4ff",
                       "marginBottom": "5px"},
            ),
            html.Div(id="metrics-bar", style={
                "display": "flex", "justifyContent": "space-around",
                "marginBottom": "10px", "backgroundColor": "#16213e",
                "padding": "10px", "borderRadius": "5px"
            }),
            dcc.Tabs([
                dcc.Tab(label="PnL Curve", value="pnl",
                        style={"backgroundColor": "#0f3460"}),
                dcc.Tab(label="Positions", value="positions",
                        style={"backgroundColor": "#0f3460"}),
                dcc.Tab(label="Limits", value="limits",
                        style={"backgroundColor": "#0f3460"}),
                dcc.Tab(label="Exposure", value="exposure",
                        style={"backgroundColor": "#0f3460"}),
                dcc.Tab(label="VaR", value="var",
                        style={"backgroundColor": "#0f3460"}),
                dcc.Tab(label="Trade Log", value="trades",
                        style={"backgroundColor": "#0f3460"}),
            ], value="pnl"),
            html.Div(id="tab-content"),
            dcc.Interval(id="refresh", interval=5000, n_intervals=0),
        ],
    )

    @app.callback(
        [Output("metrics-bar", "children"),
         Output("tab-content", "children")],
        [Input("refresh", "n_intervals"),
         Input("tabs", "value") if False else Input("refresh", "n_intervals")],
    )
    def update(n):
        client = get_client()
        pnl = client.get_pnl()
        positions = client.get_positions()

        metrics = html.Div(style={"display": "flex", "gap": "20px", "width": "100%"}, children=[
            _metric_card("Total PnL", f"${pnl.total:,.0f}", "#00ff88" if pnl.total >= 0 else "#ff4444"),
            _metric_card("Realized", f"${pnl.realized:,.0f}", "#00d4ff"),
            _metric_card("Unrealized", f"${pnl.unrealized:,.0f}", "#ffaa00"),
            _metric_card("Fees", f"${pnl.fees:,.0f}", "#888"),
            _metric_card("Positions", str(len(positions)), "#00d4ff"),
            _metric_card("Max DD", f"${pnl.max_drawdown:,.0f}", "#ff6666"),
        ])

        tab = html.Div([
            _pnl_panel(client),
        ], style={"margin-top": "10px"})

        return metrics, tab

    return app


def _metric_card(label: str, value: str, color: str) -> html.Div:
    return html.Div(style={
        "textAlign": "center", "padding": "5px 15px",
        "borderLeft": f"2px solid {color}",
    }, children=[
        html.Div(label, style={"fontSize": "11px", "color": "#888"}),
        html.Div(value, style={"fontSize": "18px", "fontWeight": "bold",
                                "color": color}),
    ])


def _pnl_panel(client: RiskEngineClient) -> html.Div:
    positions = client.get_positions()
    if not positions:
        return html.Div("No positions", style={"textAlign": "center",
                                                "padding": "40px"})

    df = pd.DataFrame([{
        "Symbol": p.symbol,
        "Net Qty": p.net_quantity,
        "Avg Entry": f"${p.avg_entry_price:.2f}",
        "Current": f"${p.current_price:.2f}",
        "Realized PnL": f"${p.realized_pnl:,.2f}",
        "Unrealized PnL": f"${p.unrealized_pnl:,.2f}",
        "Total PnL": f"${p.realized_pnl + p.unrealized_pnl:,.2f}",
        "Trades": p.trade_count,
    } for p in positions])

    pnl = client.get_pnl()

    fig = make_subplots(
        rows=1, cols=2,
        subplot_titles=("PnL by Symbol", "Position Sizes"),
        specs=[[{"type": "bar"}, {"type": "pie"}]],
    )

    symbols = [p.symbol for p in positions]
    realized = [p.realized_pnl for p in positions]
    unrealized = [p.unrealized_pnl for p in positions]

    fig.add_trace(go.Bar(name="Realized", x=symbols, y=realized,
                         marker_color="#00d4ff"), row=1, col=1)
    fig.add_trace(go.Bar(name="Unrealized", x=symbols, y=unrealized,
                         marker_color="#ffaa00"), row=1, col=1)

    net_qtys = [abs(p.net_quantity) for p in positions]
    fig.add_trace(go.Pie(
        labels=symbols, values=net_qtys,
        marker_colors=px.colors.qualitative.Set2,
    ), row=1, col=2)

    fig.update_layout(
        template="plotly_dark",
        paper_bgcolor="#1a1a2e",
        plot_bgcolor="#16213e",
        height=350,
        margin=dict(l=40, r=20, t=40, b=30),
        showlegend=True,
        legend=dict(orientation="h", y=-0.1),
    )

    return html.Div([
        dcc.Graph(figure=fig, style={"height": "380px"}),
        dash_table.DataTable(
            data=df.to_dict("records"),
            columns=[{"name": c, "id": c} for c in df.columns],
            style_table={"overflowX": "auto"},
            style_header={"backgroundColor": "#0f3460", "color": "white",
                          "fontWeight": "bold"},
            style_cell={"backgroundColor": "#1a1a2e", "color": "#e0e0e0",
                        "border": "1px solid #333", "textAlign": "center",
                        "fontSize": "13px", "padding": "5px"},
            style_data_conditional=[
                {"if": {"filter_query": '{Realized PnL} contains "-'},
                 "color": "#ff4444"},
                {"if": {"filter_query": '{Unrealized PnL} contains "-'},
                 "color": "#ff6666"},
            ],
        ),
    ])
