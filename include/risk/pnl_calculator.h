#pragma once
#include "position.h"
#include "types.h"
#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace risk {

struct PnLSnapshot {
  uint64_t timestamp_ns;
  double realized;
  double unrealized;
  double total;
  double fees;
};

struct AttributionEntry {
  std::string key;
  double realized;
  double unrealized;
  double total;
};

class PnLCalculator {
 public:
  PnLCalculator();

  static double realized_pnl(const Trade& trade, double entry_price);
  static double unrealized_pnl(double net_qty, double avg_entry,
                               double current_price);
  double total_pnl() const;

  void record_trade(const Trade& trade, double entry_price);
  void update_unrealized(const std::string& symbol, double net_qty,
                         double avg_entry, double current_price);
  void daily_pnl_reset(uint64_t cutoff_ns);
  void reset_all();

  double cumulative_realized() const;
  double cumulative_unrealized() const;
  double daily_realized() const;
  double daily_unrealized() const;
  double total_fees() const;
  double daily_fees() const;
  double max_drawdown() const;
  double current_drawdown() const;

  std::vector<AttributionEntry> attribute_by_symbol() const;
  std::vector<AttributionEntry> attribute_by_strategy() const;
  std::vector<AttributionEntry> attribute_by_hour() const;

  std::vector<PnLSnapshot> history() const;

 private:
  mutable std::mutex mu_;
  double cum_realized_;
  double cum_unrealized_;
  double cum_fees_;
  double day_realized_;
  double day_unrealized_;
  double day_fees_;
  double peak_pnl_;
  double current_pnl_;
  double max_drawdown_;
  uint64_t day_start_ns_;

  std::unordered_map<std::string, double> realized_by_symbol_;
  std::unordered_map<std::string, double> unrealized_by_symbol_;
  std::unordered_map<std::string, double> realized_by_strategy_;
  std::unordered_map<std::string, double> unrealized_by_strategy_;
  std::unordered_map<int, double> realized_by_hour_;
  std::unordered_map<int, double> unrealized_by_hour_;

  std::vector<PnLSnapshot> history_;
  std::unordered_map<std::string, double> unrealized_map_;

  void update_peak();
};

} // namespace risk
