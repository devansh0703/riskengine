#include "risk/metrics_engine.h"
#include <algorithm>
#include <cmath>
#include <numeric>
#include <random>

namespace risk {

MetricsEngine::MetricsEngine()
    : gross_exposure_(0),
      net_exposure_(0),
      total_realized_(0),
      total_unrealized_(0),
      peak_pnl_(0),
      max_drawdown_(0),
      max_gross_(1e9),
      max_net_(5e8),
      max_concentration_(0.25),
      daily_stop_loss_(1e6),
      max_drawdown_limit_(5e6) {}

void MetricsEngine::update_position(const std::string& symbol,
                                    double market_value) {
  std::lock_guard<std::mutex> lock(mu_);
  symbol_values_[symbol] = market_value;
  gross_exposure_ = 0;
  net_exposure_ = 0;
  for (auto& [s, v] : symbol_values_) {
    gross_exposure_ += std::abs(v);
    net_exposure_ += v;
  }
}

void MetricsEngine::update_pnl(double realized, double unrealized) {
  std::lock_guard<std::mutex> lock(mu_);
  total_realized_ += realized;
  total_unrealized_ += unrealized;
  double total = total_realized_ + total_unrealized_;
  if (total > peak_pnl_) peak_pnl_ = total;
  double dd = peak_pnl_ - total;
  if (dd > max_drawdown_) max_drawdown_ = dd;
}

void MetricsEngine::set_risk_params(double max_gross, double max_net,
                                    double max_concentration,
                                    double daily_stop_loss,
                                    double max_drawdown) {
  max_gross_ = max_gross;
  max_net_ = max_net;
  max_concentration_ = max_concentration;
  daily_stop_loss_ = daily_stop_loss;
  max_drawdown_limit_ = max_drawdown;
}

ExposureMetrics MetricsEngine::compute_exposure() const {
  std::lock_guard<std::mutex> lock(mu_);
  ExposureMetrics exp{};
  exp.gross_exposure = gross_exposure_;
  exp.net_exposure = net_exposure_;
  exp.long_exposure = 0;
  exp.short_exposure = 0;

  double total_sq = 0;
  for (auto& [s, v] : symbol_values_) {
    if (v > 0) exp.long_exposure += v;
    else exp.short_exposure += std::abs(v);
    exp.by_symbol[s] = v;
    double pct = gross_exposure_ > 0 ? v / gross_exposure_ : 0;
    total_sq += pct * pct;
  }
  exp.hhi_concentration = total_sq;
  return exp;
}

VaRResult MetricsEngine::compute_var(const std::vector<double>& returns) const {
  VaRResult var{};
  if (returns.size() < 2) return var;

  std::vector<double> sorted_returns = returns;
  std::sort(sorted_returns.begin(), sorted_returns.end());

  size_t n = sorted_returns.size();

  // Historical VaR
  size_t idx_95 = static_cast<size_t>(n * 0.05);
  size_t idx_99 = static_cast<size_t>(n * 0.01);
  var.historical_var_95 = -sorted_returns[std::min(idx_95, n - 1)];
  var.historical_var_99 = -sorted_returns[std::min(idx_99, n - 1)];

  // CVaR (expected shortfall)
  double sum_95 = 0, sum_99 = 0;
  for (size_t i = 0; i <= std::min(idx_95, n - 1); i++) sum_95 += sorted_returns[i];
  for (size_t i = 0; i <= std::min(idx_99, n - 1); i++) sum_99 += sorted_returns[i];
  var.cvar_95 = idx_95 > 0 ? -sum_95 / (idx_95 + 1) : 0;
  var.cvar_99 = idx_99 > 0 ? -sum_99 / (idx_99 + 1) : 0;

  // Parametric (normal assumption)
  double mean = std::accumulate(returns.begin(), returns.end(), 0.0) / n;
  double sq_sum = 0;
  for (double r : returns) sq_sum += (r - mean) * (r - mean);
  double std_dev = std::sqrt(sq_sum / n);
  var.parametric_var_95 = -(mean - 1.6449 * std_dev);
  var.parametric_var_99 = -(mean - 2.3263 * std_dev);

  // Monte Carlo
  std::mt19937_64 rng(42);
  std::normal_distribution<double> dist(mean, std_dev);
  std::vector<double> mc_returns(10000);
  for (auto& r : mc_returns) r = dist(rng);
  std::sort(mc_returns.begin(), mc_returns.end());
  size_t mc_idx_95 = static_cast<size_t>(10000 * 0.05);
  size_t mc_idx_99 = static_cast<size_t>(10000 * 0.01);
  var.monte_carlo_var_95 = -mc_returns[mc_idx_95];
  var.monte_carlo_var_99 = -mc_returns[mc_idx_99];

  var.var_95 = var.historical_var_95;
  var.var_99 = var.historical_var_99;

  return var;
}

std::vector<double> MetricsEngine::historical_simulation(
    const std::vector<double>& returns, int horizon_days) const {
  if (returns.empty()) return {};
  std::mt19937_64 rng(std::random_device{}());
  std::uniform_int_distribution<size_t> dist(0, returns.size() - 1);
  std::vector<double> simulated(10000);
  for (auto& val : simulated) {
    val = 0;
    for (int d = 0; d < horizon_days; d++) {
      val += returns[dist(rng)];
    }
  }
  return simulated;
}

GreeksResult MetricsEngine::compute_greeks(double S, double K, double T,
                                           double r, double sigma,
                                           bool is_call) const {
  if (T <= 0 || sigma <= 0 || S <= 0 || K <= 0) {
    return {0, 0, 0, 0, 0};
  }
  double sqrt_T = std::sqrt(T);
  double d1 = (std::log(S / K) + (r + 0.5 * sigma * sigma) * T) /
              (sigma * sqrt_T);
  double d2 = d1 - sigma * sqrt_T;

  auto norm_cdf = [](double x) {
    return 0.5 * (1.0 + std::erf(x / std::sqrt(2.0)));
  };
  auto norm_pdf = [](double x) {
    return std::exp(-0.5 * x * x) / std::sqrt(2.0 * M_PI);
  };

  double Nd1 = norm_cdf(d1);
  double Nd2 = norm_cdf(d2);
  double nd1 = norm_pdf(d1);

  GreeksResult g{};

  if (is_call) {
    g.delta = Nd1;
    g.theta = (-S * nd1 * sigma / (2 * sqrt_T) -
               r * K * std::exp(-r * T) * Nd2) / 365.0;
    g.rho = K * T * std::exp(-r * T) * Nd2 / 100.0;
  } else {
    g.delta = Nd1 - 1.0;
    g.theta = (-S * nd1 * sigma / (2 * sqrt_T) +
               r * K * std::exp(-r * T) * (1.0 - Nd2)) / 365.0;
    g.rho = -K * T * std::exp(-r * T) * (1.0 - Nd2) / 100.0;
  }
  g.gamma = nd1 / (S * sigma * sqrt_T);
  g.vega = S * nd1 * sqrt_T / 100.0;

  return g;
}

MarginEstimate MetricsEngine::estimate_margin(
    const ExposureMetrics& exp) const {
  MarginEstimate m{};
  m.initial_margin = exp.gross_exposure > 0 ? exp.gross_exposure * 0.15 : 0;
  m.maintenance_margin = exp.gross_exposure * 0.10;
  return m;
}

bool MetricsEngine::check_position_limit(const std::string& symbol,
                                         double market_value) const {
  std::lock_guard<std::mutex> lock(mu_);
  return std::abs(market_value) <= max_gross_;
}

bool MetricsEngine::check_daily_stop_loss(double daily_pnl) const {
  std::lock_guard<std::mutex> lock(mu_);
  return daily_pnl > -daily_stop_loss_;
}

bool MetricsEngine::check_drawdown(double current_drawdown) const {
  std::lock_guard<std::mutex> lock(mu_);
  return current_drawdown < max_drawdown_limit_;
}

} // namespace risk
