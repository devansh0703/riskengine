#include "risk/pnl_calculator.h"
#include "risk/timestamp.h"
#include <algorithm>
#include <cmath>
#include <sstream>

namespace risk {

PnLCalculator::PnLCalculator()
    : cum_realized_(0),
      cum_unrealized_(0),
      cum_fees_(0),
      day_realized_(0),
      day_unrealized_(0),
      day_fees_(0),
      peak_pnl_(0),
      current_pnl_(0),
      max_drawdown_(0),
      day_start_ns_(0) {}

double PnLCalculator::realized_pnl(const Trade& trade, double entry_price) {
  double sign = trade.side == Side::Sell ? 1.0 : -1.0;
  return sign * static_cast<double>(trade.quantity) *
         (trade.price - entry_price);
}

double PnLCalculator::unrealized_pnl(double net_qty, double avg_entry,
                                     double current_price) {
  if (net_qty == 0) return 0.0;
  double sign = net_qty > 0 ? 1.0 : -1.0;
  return sign * std::abs(net_qty) * (current_price - avg_entry);
}

double PnLCalculator::total_pnl() const {
  std::lock_guard<std::mutex> lock(mu_);
  return cum_realized_ + day_realized_ + cum_unrealized_ + day_unrealized_ -
         cum_fees_ - day_fees_;
}

void PnLCalculator::record_trade(const Trade& trade, double entry_price) {
  double rpnl = 0.0;
  if (entry_price > 0) {
    rpnl = realized_pnl(trade, entry_price);
  }
  std::lock_guard<std::mutex> lock(mu_);
  day_realized_ += rpnl;
  day_fees_ += trade.fees;
  current_pnl_ = cum_realized_ + cum_unrealized_ - cum_fees_;
  update_peak();

  auto hour = static_cast<int>(trade.timestamp_ns / 3600000000000ULL % 24);
  realized_by_symbol_[trade.symbol] += rpnl;
  realized_by_strategy_[trade.strategy] += rpnl;
  realized_by_hour_[hour] += rpnl;

  history_.push_back({trade.timestamp_ns, day_realized_, day_unrealized_,
                      day_realized_ + day_unrealized_ - day_fees_,
                      day_fees_});
}

void PnLCalculator::update_unrealized(const std::string& symbol,
                                      double net_qty, double avg_entry,
                                      double current_price) {
  double upnl = unrealized_pnl(net_qty, avg_entry, current_price);
  std::lock_guard<std::mutex> lock(mu_);
  auto old = unrealized_map_.count(symbol) ? unrealized_map_[symbol] : 0.0;
  unrealized_map_[symbol] = upnl;
  cum_unrealized_ += (upnl - old);
  day_unrealized_ += (upnl - old);
  current_pnl_ = cum_realized_ + cum_unrealized_ - cum_fees_;
  update_peak();
}

void PnLCalculator::daily_pnl_reset(uint64_t cutoff_ns) {
  std::lock_guard<std::mutex> lock(mu_);
  cum_realized_ += day_realized_;
  cum_unrealized_ += day_unrealized_;
  cum_fees_ += day_fees_;
  day_realized_ = 0.0;
  day_unrealized_ = 0.0;
  day_fees_ = 0.0;
  day_start_ns_ = cutoff_ns;
}

void PnLCalculator::reset_all() {
  std::lock_guard<std::mutex> lock(mu_);
  cum_realized_ = 0;
  cum_unrealized_ = 0;
  cum_fees_ = 0;
  day_realized_ = 0;
  day_unrealized_ = 0;
  day_fees_ = 0;
  peak_pnl_ = 0;
  current_pnl_ = 0;
  max_drawdown_ = 0;
  realized_by_symbol_.clear();
  unrealized_by_symbol_.clear();
  realized_by_strategy_.clear();
  unrealized_by_strategy_.clear();
  realized_by_hour_.clear();
  unrealized_by_hour_.clear();
  history_.clear();
  unrealized_map_.clear();
}

double PnLCalculator::cumulative_realized() const {
  std::lock_guard<std::mutex> lock(mu_);
  return cum_realized_ + day_realized_;
}

double PnLCalculator::cumulative_unrealized() const {
  std::lock_guard<std::mutex> lock(mu_);
  return cum_unrealized_ + day_unrealized_;
}

double PnLCalculator::daily_realized() const {
  std::lock_guard<std::mutex> lock(mu_);
  return day_realized_;
}

double PnLCalculator::daily_unrealized() const {
  std::lock_guard<std::mutex> lock(mu_);
  return day_unrealized_;
}

double PnLCalculator::total_fees() const {
  std::lock_guard<std::mutex> lock(mu_);
  return cum_fees_ + day_fees_;
}

double PnLCalculator::daily_fees() const {
  std::lock_guard<std::mutex> lock(mu_);
  return day_fees_;
}

double PnLCalculator::max_drawdown() const {
  std::lock_guard<std::mutex> lock(mu_);
  return max_drawdown_;
}

double PnLCalculator::current_drawdown() const {
  std::lock_guard<std::mutex> lock(mu_);
  if (peak_pnl_ <= 0) return 0.0;
  return std::max(0.0, peak_pnl_ - current_pnl_);
}

void PnLCalculator::update_peak() {
  if (current_pnl_ > peak_pnl_) peak_pnl_ = current_pnl_;
  double dd = peak_pnl_ - current_pnl_;
  if (dd > max_drawdown_) max_drawdown_ = dd;
}

std::vector<AttributionEntry> PnLCalculator::attribute_by_symbol() const {
  std::lock_guard<std::mutex> lock(mu_);
  std::vector<AttributionEntry> result;
  for (auto& [k, v] : realized_by_symbol_) {
    double upnl =
        unrealized_by_symbol_.count(k) ? unrealized_by_symbol_.at(k) : 0.0;
    result.push_back({k, v, upnl, v + upnl});
  }
  return result;
}

std::vector<AttributionEntry> PnLCalculator::attribute_by_strategy() const {
  std::lock_guard<std::mutex> lock(mu_);
  std::vector<AttributionEntry> result;
  for (auto& [k, v] : realized_by_strategy_) {
    double upnl =
        unrealized_by_strategy_.count(k) ? unrealized_by_strategy_.at(k) : 0.0;
    result.push_back({k, v, upnl, v + upnl});
  }
  return result;
}

std::vector<AttributionEntry> PnLCalculator::attribute_by_hour() const {
  std::lock_guard<std::mutex> lock(mu_);
  std::vector<AttributionEntry> result;
  for (auto& [k, v] : realized_by_hour_) {
    double upnl =
        unrealized_by_hour_.count(k) ? unrealized_by_hour_.at(k) : 0.0;
    std::ostringstream key;
    key << std::setfill('0') << std::setw(2) << k << ":00";
    result.push_back({key.str(), v, upnl, v + upnl});
  }
  return result;
}

std::vector<PnLSnapshot> PnLCalculator::history() const {
  std::lock_guard<std::mutex> lock(mu_);
  return history_;
}

} // namespace risk
