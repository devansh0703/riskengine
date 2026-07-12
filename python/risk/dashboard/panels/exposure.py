"""Exposure heatmap panel."""
import plotly.graph_objects as go
import plotly.express as px
from dash import html
import pandas as pd
import numpy as np
from risk.client import RiskEngineClient


def render_exposure(client: RiskEngineClient) -> html.Div:
    positions = client.get_positions()

    if not positions:
        return html.Div("No exposure data", style={
            "textAlign": "center", "padding": "40px", "color": "#888"
        })

    symbols = [p.symbol for p in positions]
    values = [abs(p.net_quantity * p.current_price) for p in positions]
    net_values = [p.net_quantity * p.current_price for p in positions]
    total = sum(values) or 1
    weights = [v / total * 100 for v in values]

    fig_bar = go.Figure()
    fig_bar.add_trace(go.Bar(
        x=symbols, y=net_values,
        marker_color=["#00ff88" if v > 0 else "#ff4444" for v in net_values],
        text=[f"${v:,.0f}" for v in net_values],
        textposition="outside",
    ))
    fig_bar.update_layout(
        template="plotly_dark",
        paper_bgcolor="#1a1a2e",
        plot_bgcolor="#16213e",
        title="Net Exposure by Symbol",
        title_font_color="#00d4ff",
        height=300,
        margin=dict(l=50, r=20, t=40, b=30),
        yaxis_title="Net Exposure ($)",
    )

    fig_heat = go.Figure(go.Heatmap(
        z=[weights],
        x=symbols,
        y=["Weight"],
        colorscale="Viridis",
        text=[f"{w:.1f}%" for w in weights],
        texttemplate="%{text}",
        showscale=True,
        colorbar=dict(title="%"),
    ))
    fig_heat.update_layout(
        template="plotly_dark",
        paper_bgcolor="#1a1a2e",
        plot_bgcolor="#16213e",
        title="Concentration Heatmap",
        title_font_color="#00d4ff",
        height=150,
        margin=dict(l=50, r=20, t=40, b=30),
    )

    hhi = sum((w / 100) ** 2 for w in weights)

    return html.Div([
        html.H3("Exposure Analysis", style={"color": "#00d4ff", "marginBottom": "10px"}),
        html.Div(style={"display": "flex", "gap": "20px", "marginBottom": "10px"}, children=[
            _stat("Gross Exposure", f"${sum(values):,.0f}", "#ffaa00"),
            _stat("Net Exposure", f"${sum(net_values):,.0f}", "#00d4ff"),
            _stat("HHI Index", f"{hhi:.4f}", "#00ff88" if hhi < 0.25 else "#ff4444"),
            _stat("Symbols", str(len(symbols)), "#e0e0e0"),
        ]),
        dcc.Graph(figure=fig_bar, config={"displayModeBar": False},
                  style={"height": "320px"}),
        dcc.Graph(figure=fig_heat, config={"displayModeBar": False},
                  style={"height": "170px"}),
    ])


def _stat(label: str, value: str, color: str) -> html.Div:
    return html.Div(style={
        "textAlign": "center", "padding": "8px 16px",
        "backgroundColor": "#16213e", "borderRadius": "5px",
        "borderLeft": f"3px solid {color}",
    }, children=[
        html.Div(label, style={"fontSize": "11px", "color": "#888"}),
        html.Div(value, style={"fontSize": "16px", "fontWeight": "bold",
                                "color": color}),
    ])


from dash import dcc  # noqa: E402
