"""PnL Curve panel."""
import plotly.graph_objects as go
from plotly.subplots import make_subplots
import pandas as pd
import numpy as np
from risk.client import RiskEngineClient


def render_pnl_curve(client: RiskEngineClient) -> go.Figure:
    """Render PnL curve with drawdown overlay."""
    fig = make_subplots(
        rows=2, cols=1,
        shared_xaxes=True,
        vertical_spacing=0.08,
        row_heights=[0.7, 0.3],
        subplot_titles=("Cumulative PnL", "Drawdown"),
    )

    pnl = client.get_pnl()
    positions = client.get_positions()
    total = pnl.realized + pnl.unrealized

    fig.add_trace(
        go.Scatter(
            x=[0], y=[pnl.realized],
            name="Realized", line=dict(color="#00d4ff", width=2),
            fill="tozeroy", fillcolor="rgba(0,212,255,0.1)",
        ),
        row=1, col=1,
    )
    fig.add_trace(
        go.Scatter(
            x=[0], y=[total],
            name="Total", line=dict(color="#00ff88", width=2),
        ),
        row=1, col=1,
    )

    dd = pnl.max_drawdown
    fig.add_trace(
        go.Bar(
            x=[0], y=[-dd],
            name="Drawdown", marker_color="#ff4444",
        ),
        row=2, col=1,
    )

    fig.update_layout(
        template="plotly_dark",
        paper_bgcolor="#1a1a2e",
        plot_bgcolor="#16213e",
        height=350,
        margin=dict(l=50, r=20, t=30, b=30),
        legend=dict(orientation="h", y=-0.15),
    )
    fig.update_yaxes(title_text="PnL ($)", row=1, col=1)
    fig.update_yaxes(title_text="Drawdown ($)", row=2, col=1)

    return fig
