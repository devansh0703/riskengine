#include "risk/lot_matching.h"
#include <algorithm>
#include <cmath>

namespace risk {

LotMatching::LotMatching(LotMatchMethod method) : method_(method) {}

LotMatchResult LotMatching::match(const Trade& trade) {
  std::lock_guard<std::mutex> lock(mu_);
  auto& lots = lots_[trade.symbol];
  switch (method_) {
    case LotMatchMethod::FIFO:
      return match_fifo(trade, lots);
    case LotMatchMethod::LIFO:
      return match_lifo(trade, lots);
    case LotMatchMethod::AverageCost:
      return match_avg_cost(trade, lots);
  }
  return match_fifo(trade, lots);
}

LotMatchResult LotMatching::match_fifo(const Trade& trade,
                                       std::deque<Lot>& lots) {
  LotMatchResult result{0.0, {}};
  int64_t qty_remaining = static_cast<int64_t>(trade.quantity);
  bool is_sell = trade.side == Side::Sell;

  while (qty_remaining > 0 && !lots.empty()) {
    auto& front = lots.front();
    int64_t match_qty = std::min(qty_remaining, front.remaining);
    double pnl = 0.0;
    if (is_sell) {
      pnl = static_cast<double>(match_qty) * (trade.price - front.price);
    } else {
      pnl = static_cast<double>(match_qty) * (front.price - trade.price);
    }
    result.realized_pnl += pnl;
    result.matched_pairs.push_back({front.price, match_qty});
    front.remaining -= match_qty;
    qty_remaining -= match_qty;
    if (front.remaining == 0) lots.pop_front();
  }

  if (qty_remaining > 0) {
    lots.push_front({trade.trade_id, trade.timestamp_ns,
                     qty_remaining, trade.price, qty_remaining});
  } else if (trade.side == Side::Buy && qty_remaining == 0) {
    lots.push_back({trade.trade_id, trade.timestamp_ns,
                    static_cast<int64_t>(trade.quantity), trade.price,
                    static_cast<int64_t>(trade.quantity)});
  }

  return result;
}

LotMatchResult LotMatching::match_lifo(const Trade& trade,
                                       std::deque<Lot>& lots) {
  LotMatchResult result{0.0, {}};
  int64_t qty_remaining = static_cast<int64_t>(trade.quantity);
  bool is_sell = trade.side == Side::Sell;

  while (qty_remaining > 0 && !lots.empty()) {
    auto& back = lots.back();
    int64_t match_qty = std::min(qty_remaining, back.remaining);
    double pnl = 0.0;
    if (is_sell) {
      pnl = static_cast<double>(match_qty) * (trade.price - back.price);
    } else {
      pnl = static_cast<double>(match_qty) * (back.price - trade.price);
    }
    result.realized_pnl += pnl;
    result.matched_pairs.push_back({back.price, match_qty});
    back.remaining -= match_qty;
    qty_remaining -= match_qty;
    if (back.remaining == 0) lots.pop_back();
  }

  if (qty_remaining > 0) {
    lots.push_back({trade.trade_id, trade.timestamp_ns,
                    qty_remaining, trade.price, qty_remaining});
  } else if (trade.side == Side::Buy && qty_remaining == 0) {
    lots.push_back({trade.trade_id, trade.timestamp_ns,
                    static_cast<int64_t>(trade.quantity), trade.price,
                    static_cast<int64_t>(trade.quantity)});
  }

  return result;
}

LotMatchResult LotMatching::match_avg_cost(const Trade& trade,
                                           std::deque<Lot>& lots) {
  LotMatchResult result{0.0, {}};

  int64_t total_qty = 0;
  double total_cost = 0.0;
  for (auto& lot : lots) {
    total_qty += lot.remaining;
    total_cost += lot.remaining * lot.price;
  }

  bool is_buy = trade.side == Side::Buy;
  int64_t new_qty = is_buy ? static_cast<int64_t>(trade.quantity)
                           : -static_cast<int64_t>(trade.quantity);
  int64_t combined = total_qty + new_qty;

  if (combined == 0) {
    double avg = total_qty > 0 ? total_cost / total_qty : 0.0;
    result.realized_pnl =
        static_cast<double>(std::abs(new_qty)) * (trade.price - avg);
    result.matched_pairs.push_back({avg, std::abs(new_qty)});
    lots.clear();
    return result;
  }

  bool same_dir = (total_qty > 0 && new_qty > 0) ||
                  (total_qty < 0 && new_qty < 0);

  if (same_dir) {
    double new_total_cost = total_cost + static_cast<double>(new_qty) * trade.price;
    double avg = combined != 0 ? new_total_cost / static_cast<double>(combined) : trade.price;
    lots.clear();
    lots.push_back({trade.trade_id, trade.timestamp_ns, std::abs(combined),
                    avg, std::abs(combined)});
  } else if (total_qty != 0) {
    double avg = total_cost / static_cast<double>(total_qty);
    int64_t matched = std::min(std::abs(new_qty), std::abs(total_qty));
    if (total_qty > 0) {
      result.realized_pnl =
          static_cast<double>(matched) * (trade.price - avg);
    } else {
      result.realized_pnl =
          static_cast<double>(matched) * (avg - trade.price);
    }
    result.matched_pairs.push_back({avg, matched});
    lots.clear();
    if (std::abs(combined) > 0) {
      double remaining_avg = (std::abs(combined) == std::abs(total_qty))
          ? avg
          : (total_cost - (total_qty > 0 ? 1.0 : -1.0) * matched * avg) /
                static_cast<double>(std::abs(combined));
      (void)remaining_avg;
      lots.push_back({trade.trade_id, trade.timestamp_ns, std::abs(combined),
                      avg, std::abs(combined)});
    }
  } else {
    lots.push_back({trade.trade_id, trade.timestamp_ns, std::abs(new_qty),
                    trade.price, std::abs(new_qty)});
  }

  return result;
}

std::vector<Lot> LotMatching::remaining_lots(
    const std::string& symbol) const {
  std::lock_guard<std::mutex> lock(mu_);
  auto it = lots_.find(symbol);
  if (it != lots_.end()) return {it->second.begin(), it->second.end()};
  return {};
}

void LotMatching::clear(const std::string& symbol) {
  std::lock_guard<std::mutex> lock(mu_);
  lots_.erase(symbol);
}

void LotMatching::clear_all() {
  std::lock_guard<std::mutex> lock(mu_);
  lots_.clear();
}

} // namespace risk
