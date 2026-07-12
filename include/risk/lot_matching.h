#pragma once
#include "types.h"
#include <cstdint>
#include <deque>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace risk {

struct Lot {
  uint64_t trade_id;
  uint64_t timestamp_ns;
  int64_t quantity;
  double price;
  int64_t remaining;
};

struct LotMatchResult {
  double realized_pnl;
  std::vector<std::pair<double, int64_t>> matched_pairs; // (price, qty)
};

class LotMatching {
 public:
  explicit LotMatching(LotMatchMethod method = LotMatchMethod::FIFO);

  LotMatchResult match(const Trade& trade);
  std::vector<Lot> remaining_lots(const std::string& symbol) const;
  void clear(const std::string& symbol);
  void clear_all();

  LotMatchMethod method() const { return method_; }
  void set_method(LotMatchMethod m) { method_ = m; }

 private:
  LotMatchMethod method_;
  mutable std::mutex mu_;
  std::unordered_map<std::string, std::deque<Lot>> lots_;

  LotMatchResult match_fifo(const Trade& trade, std::deque<Lot>& lots);
  LotMatchResult match_lifo(const Trade& trade, std::deque<Lot>& lots);
  LotMatchResult match_avg_cost(const Trade& trade, std::deque<Lot>& lots);
};

} // namespace risk
