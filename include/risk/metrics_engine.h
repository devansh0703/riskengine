#pragma once
#include "position.h"
#include "types.h"
#include <cstdint>
#include <mutex>
#include <string>

#include <unordered_map>
#include <vector>

namespace risk {

struct ExposureMetrics {
  double gross_exposure;
  double net_exposure;
  double long_exposure;
  double short_exposure;
  double hhi_concentration;
  std::unordered_map<std::string, double> by_symbol;
  std::unordered_map<std::string, double> by_strategy;
};

struct VaRResult {
  double var_95;
  double var_99;
  double cvar_95;
  double cvar_99;
  double historical_var_95;
  double historical_var_99;
  double parametric_var_95;
  double parametric_var_99;
  double monte_carlo_var_95;
  double monte_carlo_var_99;
};

struct GreeksResult {
  double delta;
  double gamma;
  double vega;
  double theta;
  double rho;
};

struct MarginEstimate {
  double initial_margin;
  double maintenance_margin;
};

class MetricsEngine {
 public:
  MetricsEngine();

  void update_position(const std::string& symbol, double market_value);
  void update_pnl(double realized, double unrealized);
  void set_risk_params(double max_gross, double max_net,
                       double max_concentration, double daily_stop_loss,
                       double max_drawdown);

  ExposureMetrics compute_exposure() const;
  VaRResult compute_var(const std::vector<double>& returns) const;
  std::vector<double> historical_simulation(const std::vector<double>& returns,
                                            int horizon_days) const;
  GreeksResult compute_greeks(double S, double K, double T, double r,
                              double sigma, bool is_call) const;
  MarginEstimate estimate_margin(const ExposureMetrics& exp) const;

  bool check_position_limit(const std::string& symbol,
                            double market_value) const;
  bool check_daily_stop_loss(double daily_pnl) const;
  bool check_drawdown(double current_drawdown) const;

 private:
  mutable std::mutex mu_;
  std::unordered_map<std::string, double> symbol_values_;
  double gross_exposure_;
  double net_exposure_;
  double total_realized_;
  double total_unrealized_;
  double peak_pnl_;
  double max_drawdown_;

  double max_gross_;
  double max_net_;
  double max_concentration_;
  double daily_stop_loss_;
  double max_drawdown_limit_;
};

} // namespace risk
