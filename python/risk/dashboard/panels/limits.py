"""Risk Limits gauge panel."""
import plotly.graph_objects as go
from dash import html
import dash_daq as daq
from risk.client import RiskEngineClient


def render_limits(client: RiskEngineClient) -> html.Div:
    pnl = client.get_pnl()
    positions = client.get_positions()

    total_exposure = sum(
        abs(p.net_quantity * p.current_price) for p in positions
    )
    net_exposure = sum(
        p.net_quantity * p.current_price for p in positions
    )

    max_exposure = 1e9
    exposure_pct = min(100, total_exposure / max_exposure * 100)

    max_daily_loss = 500000
    daily_loss_used = min(100, max(0, -pnl.realized) / max_daily_loss * 100)

    max_drawdown = 1000000
    dd_pct = min(100, pnl.max_drawdown / max_drawdown * 100)

    def make_gauge(value, title, color_ranges):
        return go.Figure(go.Indicator(
            mode="gauge+number",
            value=value,
            title={"text": title, "font": {"size": 14, "color": "#e0e0e0"}},
            number={"suffix": "%", "font": {"size": 24}},
            gauge={
                "axis": {"range": [0, 100], "tickwidth": 1},
                "bar": {"color": "#00d4ff"},
                "steps": color_ranges,
                "threshold": {
                    "line": {"color": "red", "width": 4},
                    "thickness": 0.75,
                    "value": value,
                },
            },
        ))

    fig_exp = make_gauge(exposure_pct, "Gross Exposure Utilization", [
        {"range": [0, 50], "color": "rgba(0,255,136,0.2)"},
        {"range": [50, 80], "color": "rgba(255,170,0,0.2)"},
        {"range": [80, 100], "color": "rgba(255,68,68,0.2)"},
    ])
    fig_exp.update_layout(
        template="plotly_dark", paper_bgcolor="#1a1a2e",
        height=220, margin=dict(l=30, r=30, t=40, b=20),
    )

    fig_loss = make_gauge(daily_loss_used, "Daily Stop-Loss Used", [
        {"range": [0, 30], "color": "rgba(0,255,136,0.2)"},
        {"range": [30, 70], "color": "rgba(255,170,0,0.2)"},
        {"range": [70, 100], "color": "rgba(255,68,68,0.2)"},
    ])
    fig_loss.update_layout(
        template="plotly_dark", paper_bgcolor="#1a1a2e",
        height=220, margin=dict(l=30, r=30, t=40, b=20),
    )

    fig_dd = make_gauge(dd_pct, "Max Drawdown Utilized", [
        {"range": [0, 25], "color": "rgba(0,255,136,0.2)"},
        {"range": [25, 60], "color": "rgba(255,170,0,0.2)"},
        {"range": [60, 100], "color": "rgba(255,68,68,0.2)"},
    ])
    fig_dd.update_layout(
        template="plotly_dark", paper_bgcolor="#1a1a2e",
        height=220, margin=dict(l=30, r=30, t=40, b=20),
    )

    return html.Div([
        html.H3("Risk Limits", style={"color": "#00d4ff", "marginBottom": "10px"}),
        html.Div(style={"display": "flex", "gap": "10px"}, children=[
            html.Div([
                html.Div(f"${total_exposure:,.0f}", style={
                    "textAlign": "center", "fontSize": "14px", "color": "#ffaa00",
                    "marginBottom": "5px"
                }),
                html.Div(f"Net: ${net_exposure:,.0f}", style={
                    "textAlign": "center", "fontSize": "11px", "color": "#888"
                }),
                html.Div(style={"height": "5px"}),
            ]),
        ]),
        html.Div(style={"display": "flex", "gap": "10px"}, children=[
            html.Div(style={"flex": 1}, children=[
                html.Div(dcc.Graph(figure=fig_exp, config={"displayModeBar": False})),
            ]),
            html.Div(style={"flex": 1}, children=[
                html.Div(dcc.Graph(figure=fig_loss, config={"displayModeBar": False})),
            ]),
            html.Div(style={"flex": 1}, children=[
                html.Div(dcc.Graph(figure=fig_dd, config={"displayModeBar": False})),
            ]),
        ]),
    ])
