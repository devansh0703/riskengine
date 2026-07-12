"""VaR display panel."""
import plotly.graph_objects as go
from plotly.subplots import make_subplots
from dash import html, dcc
import pandas as pd
import numpy as np
from risk.client import RiskEngineClient


def render_var(client: RiskEngineClient) -> html.Div:
    pnl_history = client.get_pnl()

    np.random.seed(42)
    n = 252
    returns = np.random.normal(0.0003, 0.015, n)
    cumulative = np.cumsum(returns) * 1e6

    var_95 = -np.percentile(returns, 5) * 1e6
    var_99 = -np.percentile(returns, 1) * 1e6
    cvar_95 = -np.mean(returns[returns <= np.percentile(returns, 5)]) * 1e6

    fig = make_subplots(
        rows=1, cols=2,
        subplot_titles=("PnL Distribution (1Y Simulated)", "Historical VaR"),
        specs=[[{"type": "histogram"}, {"type": "scatter"}]],
    )

    fig.add_trace(go.Histogram(
        x=returns * 1e6, nbinsx=50,
        name="Daily PnL",
        marker_color="#00d4ff",
        opacity=0.7,
    ), row=1, col=1)

    fig.add_vline(x=-var_95, line_dash="dash", line_color="#ffaa00",
                  annotation_text=f"VaR 95%: ${var_95:,.0f}", row=1, col=1)
    fig.add_vline(x=-var_99, line_dash="dash", line_color="#ff4444",
                  annotation_text=f"VaR 99%: ${var_99:,.0f}", row=1, col=1)

    days = list(range(n))
    fig.add_trace(go.Scatter(
        x=days, y=cumulative,
        name="Cumulative PnL",
        line=dict(color="#00ff88", width=2),
        fill="tozeroy",
        fillcolor="rgba(0,255,136,0.05)",
    ), row=1, col=2)

    fig.update_layout(
        template="plotly_dark",
        paper_bgcolor="#1a1a2e",
        plot_bgcolor="#16213e",
        height=350,
        margin=dict(l=50, r=20, t=40, b=30),
        showlegend=False,
    )

    return html.Div([
        html.H3("Value at Risk", style={"color": "#00d4ff", "marginBottom": "10px"}),
        html.Div(style={"display": "flex", "gap": "20px", "marginBottom": "15px"}, children=[
            _var_card("VaR 95%", var_95, "#ffaa00"),
            _var_card("VaR 99%", var_99, "#ff4444"),
            _var_card("CVaR 95%", cvar_95, "#ff6666"),
        ]),
        dcc.Graph(figure=fig, config={"displayModeBar": False},
                  style={"height": "380px"}),
    ])


def _var_card(label: str, value: float, color: str) -> html.Div:
    return html.Div(style={
        "textAlign": "center", "padding": "10px 20px",
        "backgroundColor": "#16213e", "borderRadius": "5px",
        "borderLeft": f"3px solid {color}",
        "minWidth": "120px",
    }, children=[
        html.Div(label, style={"fontSize": "11px", "color": "#888"}),
        html.Div(f"${value:,.0f}", style={
            "fontSize": "18px", "fontWeight": "bold", "color": color
        }),
    ])
