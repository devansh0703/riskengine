#pragma once
#include "position.h"
#include "types.h"
#include <functional>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace risk {

class PositionTracker {
 public:
  PositionTracker();

  void update_position(const Trade& trade);
  PositionDetail get_position(const std::string& symbol) const;
  std::vector<PositionDetail> get_all_positions() const;
  std::vector<TimestampedPosition> get_position_history(
      const std::string& symbol) const;

  void set_price(const std::string& symbol, double price);
  void set_lot_match_method(LotMatchMethod method);

  using PositionUpdateCallback =
      std::function<void(const PositionDetail&)>;
  void on_position_update(PositionUpdateCallback cb);

  size_t symbol_count() const;

 private:
  mutable std::mutex mu_;
  std::unordered_map<std::string, PositionDetail> positions_;
  std::unordered_map<std::string, std::vector<TimestampedPosition>> history_;
  LotMatchMethod lot_method_;
  PositionUpdateCallback callback_;
  double realized_pnl_for_trade(const Trade& trade,
                                PositionDetail& pos);
};

} // namespace risk
