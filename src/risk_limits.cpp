#include "risk/risk_limits.h"
#include <sstream>

namespace risk {

RiskLimits::RiskLimits()
    : loss_limit_{0, 0, 0}, exposure_limit_{0, 0, 0} {}

void RiskLimits::add_position_limit(const PositionLimit& limit) {
  position_limits_.push_back(limit);
}

void RiskLimits::set_loss_limit(const LossLimit& limit) {
  loss_limit_ = limit;
}

void RiskLimits::set_exposure_limit(const ExposureLimit& limit) {
  exposure_limit_ = limit;
}

std::vector<LimitBreach> RiskLimits::check_position(
    int64_t net_qty, double current_price, const std::string& symbol) const {
  std::vector<LimitBreach> breaches;

  for (auto& lim : position_limits_) {
    if (!lim.symbol.empty() && lim.symbol != symbol) continue;

    double market_val = static_cast<double>(net_qty) * current_price;

    if (net_qty > 0 && net_qty > lim.max_long) {
      std::ostringstream msg;
      msg << "Long position " << net_qty << " exceeds limit "
          << lim.max_long << " for " << symbol;
      breaches.push_back({"position", symbol, static_cast<double>(net_qty),
                          static_cast<double>(lim.max_long), lim.hard,
                          msg.str()});
    }
    if (net_qty < 0 && std::abs(net_qty) > lim.max_short) {
      std::ostringstream msg;
      msg << "Short position " << std::abs(net_qty) << " exceeds limit "
          << lim.max_short << " for " << symbol;
      breaches.push_back(
          {"position", symbol, static_cast<double>(std::abs(net_qty)),
           static_cast<double>(lim.max_short), lim.hard, msg.str()});
    }
  }
  return breaches;
}

std::vector<LimitBreach> RiskLimits::check_loss(double daily_pnl,
                                                double drawdown) const {
  std::vector<LimitBreach> breaches;

  if (loss_limit_.daily_stop_loss > 0 &&
      daily_pnl < -loss_limit_.daily_stop_loss) {
    std::ostringstream msg;
    msg << "Daily PnL " << daily_pnl << " breaches stop loss "
        << -loss_limit_.daily_stop_loss;
    breaches.push_back({"daily_stop_loss", "", daily_pnl,
                        -loss_limit_.daily_stop_loss, true, msg.str()});
  }

  if (loss_limit_.max_drawdown > 0 &&
      drawdown > loss_limit_.max_drawdown) {
    std::ostringstream msg;
    msg << "Drawdown " << drawdown << " exceeds max "
        << loss_limit_.max_drawdown;
    breaches.push_back(
        {"max_drawdown", "", drawdown, loss_limit_.max_drawdown, true,
         msg.str()});
  }

  return breaches;
}

std::vector<LimitBreach> RiskLimits::check_exposure(
    double gross, double net, double concentration) const {
  std::vector<LimitBreach> breaches;

  if (exposure_limit_.max_gross > 0 && gross > exposure_limit_.max_gross) {
    std::ostringstream msg;
    msg << "Gross exposure " << gross << " exceeds limit "
        << exposure_limit_.max_gross;
    breaches.push_back({"gross_exposure", "", gross,
                        exposure_limit_.max_gross, true, msg.str()});
  }

  if (exposure_limit_.max_net > 0 &&
      std::abs(net) > exposure_limit_.max_net) {
    std::ostringstream msg;
    msg << "Net exposure " << net << " exceeds limit "
        << exposure_limit_.max_net;
    breaches.push_back(
        {"net_exposure", "", std::abs(net), exposure_limit_.max_net, true,
         msg.str()});
  }

  if (exposure_limit_.max_concentration > 0 &&
      concentration > exposure_limit_.max_concentration) {
    std::ostringstream msg;
    msg << "Concentration " << concentration << " exceeds limit "
        << exposure_limit_.max_concentration;
    breaches.push_back({"concentration", "", concentration,
                        exposure_limit_.max_concentration, false,
                        msg.str()});
  }

  return breaches;
}

bool RiskLimits::has_hard_breach(const std::vector<LimitBreach>& breaches) const {
  for (auto& b : breaches)
    if (b.is_hard) return true;
  return false;
}

void RiskLimits::set_position_limit(const std::string& symbol,
                                    int64_t max_long, int64_t max_short,
                                    bool hard) {
  for (auto& lim : position_limits_) {
    if (lim.symbol == symbol) {
      lim.max_long = max_long;
      lim.max_short = max_short;
      lim.hard = hard;
      return;
    }
  }
  position_limits_.push_back({symbol, max_long, max_short, hard});
}

void RiskLimits::set_daily_stop_loss(double amount) {
  loss_limit_.daily_stop_loss = amount;
}

void RiskLimits::set_max_drawdown(double amount) {
  loss_limit_.max_drawdown = amount;
}

} // namespace risk
