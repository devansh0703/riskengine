#pragma once
#include "types.h"
#include "timestamp.h"
#include <cstdint>
#include <string>
#include <vector>

namespace risk {

struct PositionDetail {
  std::string symbol;
  int64_t net_quantity;
  double avg_entry_price;
  double realized_pnl;
  double unrealized_pnl;
  double current_price;
  uint64_t trade_count;
  uint64_t last_update_ns;

  double market_value() const {
    return static_cast<double>(net_quantity) * current_price;
  }

  double cost_basis() const {
    return static_cast<double>(net_quantity) * avg_entry_price;
  }

  double total_pnl() const { return realized_pnl + unrealized_pnl; }

  void update_unrealized() {
    if (net_quantity == 0) {
      unrealized_pnl = 0.0;
      return;
    }
    double sign = net_quantity > 0 ? 1.0 : -1.0;
    unrealized_pnl = sign * static_cast<double>(net_quantity) *
                     (current_price - avg_entry_price);
  }
};

struct TimestampedPosition {
  uint64_t timestamp_ns;
  PositionDetail position;
};

} // namespace risk
