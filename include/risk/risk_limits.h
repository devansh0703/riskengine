#pragma once
#include "types.h"
#include <cstdint>
#include <string>
#include <vector>

namespace risk {

struct PositionLimit {
  std::string symbol;
  int64_t max_long;
  int64_t max_short;
  bool hard;
};

struct LossLimit {
  double daily_stop_loss;
  double max_drawdown;
  int max_drawdown_duration_minutes;
};

struct ExposureLimit {
  double max_gross;
  double max_net;
  double max_concentration;
};

struct LimitBreach {
  std::string limit_type;
  std::string symbol;
  double current_value;
  double limit_value;
  bool is_hard;
  std::string message;
};

class RiskLimits {
 public:
  RiskLimits();

  void add_position_limit(const PositionLimit& limit);
  void set_loss_limit(const LossLimit& limit);
  void set_exposure_limit(const ExposureLimit& limit);

  std::vector<LimitBreach> check_position(int64_t net_qty,
                                          double current_price,
                                          const std::string& symbol) const;
  std::vector<LimitBreach> check_loss(double daily_pnl,
                                      double drawdown) const;
  std::vector<LimitBreach> check_exposure(double gross,
                                          double net,
                                          double concentration) const;

  bool has_hard_breach(const std::vector<LimitBreach>& breaches) const;

  void set_position_limit(const std::string& symbol, int64_t max_long,
                          int64_t max_short, bool hard);
  void set_daily_stop_loss(double amount);
  void set_max_drawdown(double amount);

 private:
  std::vector<PositionLimit> position_limits_;
  LossLimit loss_limit_;
  ExposureLimit exposure_limit_;
};

} // namespace risk
