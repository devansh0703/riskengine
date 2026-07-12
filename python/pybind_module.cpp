#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include "risk/types.h"
#include "risk/position_tracker.h"
#include "risk/pnl_calculator.h"
#include "risk/lot_matching.h"
#include "risk/metrics_engine.h"
#include "risk/risk_limits.h"
#include "risk/alert_manager.h"
#include "risk/trade_journal.h"
#include "risk/market_data.h"
#include "greeks/black_scholes.h"

namespace py = pybind11;

PYBIND11_MODULE(risk_python, m) {
    m.doc() = "RiskEngine Python bindings";

    py::enum_<risk::Side>(m, "Side")
        .value("Buy", risk::Side::Buy)
        .value("Sell", risk::Side::Sell);

    py::enum_<risk::LotMatchMethod>(m, "LotMatchMethod")
        .value("FIFO", risk::LotMatchMethod::FIFO)
        .value("LIFO", risk::LotMatchMethod::LIFO)
        .value("AverageCost", risk::LotMatchMethod::AverageCost);

    py::class_<risk::Trade>(m, "Trade")
        .def(py::init<>())
        .def_readwrite("trade_id", &risk::Trade::trade_id)
        .def_readwrite("timestamp_ns", &risk::Trade::timestamp_ns)
        .def_readwrite("symbol", &risk::Trade::symbol)
        .def_readwrite("side", &risk::Trade::side)
        .def_readwrite("quantity", &risk::Trade::quantity)
        .def_readwrite("price", &risk::Trade::price)
        .def_readwrite("fees", &risk::Trade::fees)
        .def_readwrite("strategy", &risk::Trade::strategy)
        .def_readwrite("account", &risk::Trade::account)
        .def_readwrite("fill_id", &risk::Trade::fill_id);

    py::class_<risk::PositionDetail>(m, "PositionDetail")
        .def_readonly("symbol", &risk::PositionDetail::symbol)
        .def_readonly("net_quantity", &risk::PositionDetail::net_quantity)
        .def_readonly("avg_entry_price", &risk::PositionDetail::avg_entry_price)
        .def_readonly("realized_pnl", &risk::PositionDetail::realized_pnl)
        .def_readonly("unrealized_pnl", &risk::PositionDetail::unrealized_pnl)
        .def_readonly("current_price", &risk::PositionDetail::current_price)
        .def_readonly("trade_count", &risk::PositionDetail::trade_count);

    py::class_<risk::PositionTracker>(m, "PositionTracker")
        .def(py::init<>())
        .def("update_position", &risk::PositionTracker::update_position)
        .def("get_position", &risk::PositionTracker::get_position)
        .def("get_all_positions", &risk::PositionTracker::get_all_positions)
        .def("set_price", &risk::PositionTracker::set_price)
        .def("symbol_count", &risk::PositionTracker::symbol_count);

    py::class_<risk::PnLCalculator>(m, "PnLCalculator")
        .def(py::init<>())
        .def_static("realized_pnl", &risk::PnLCalculator::realized_pnl)
        .def_static("unrealized_pnl", &risk::PnLCalculator::unrealized_pnl)
        .def("record_trade", &risk::PnLCalculator::record_trade)
        .def("total_pnl", &risk::PnLCalculator::total_pnl)
        .def("cumulative_realized", &risk::PnLCalculator::cumulative_realized)
        .def("daily_realized", &risk::PnLCalculator::daily_realized);

    py::class_<risk::MetricsEngine>(m, "MetricsEngine")
        .def(py::init<>())
        .def("compute_exposure", &risk::MetricsEngine::compute_exposure)
        .def("compute_var", &risk::MetricsEngine::compute_var)
        .def("compute_greeks", &risk::MetricsEngine::compute_greeks);

    m.def("call_price", &risk::greeks::call_price);
    m.def("put_price", &risk::greeks::put_price);
    m.def("delta_call", &risk::greeks::delta_call);
    m.def("delta_put", &risk::greeks::delta_put);
    m.def("gamma", &risk::greeks::gamma);
    m.def("vega", &risk::greeks::vega);
    m.def("theta_call", &risk::greeks::theta_call);
    m.def("theta_put", &risk::greeks::theta_put);
    m.def("implied_volatility", &risk::greeks::implied_volatility);
}
