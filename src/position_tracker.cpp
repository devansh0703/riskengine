#include "risk/position_tracker.h"
#include "risk/lot_matching.h"
#include "risk/timestamp.h"

namespace risk {

PositionTracker::PositionTracker()
    : lot_method_(LotMatchMethod::FIFO), callback_(nullptr) {}

void PositionTracker::update_position(const Trade& trade) {
  std::lock_guard<std::mutex> lock(mu_);

  auto& pos = positions_[trade.symbol];
  if (pos.symbol.empty()) {
    pos.symbol = trade.symbol;
    pos.net_quantity = 0;
    pos.avg_entry_price = 0.0;
    pos.realized_pnl = 0.0;
    pos.unrealized_pnl = 0.0;
    pos.current_price = 0.0;
    pos.trade_count = 0;
    pos.last_update_ns = trade.timestamp_ns;
  }

  int64_t signed_qty = side_sign(trade.side) * trade.quantity;
  int64_t old_net = pos.net_quantity;
  pos.net_quantity += signed_qty;
  pos.trade_count++;
  pos.last_update_ns = trade.timestamp_ns;

  double pnl = realized_pnl_for_trade(trade, pos);
  pos.realized_pnl += pnl;

  if (pos.net_quantity != 0) {
    if (old_net == 0) {
      pos.avg_entry_price = trade.price;
    } else if (static_cast<int64_t>(
                   static_cast<uint64_t>(old_net ^ signed_qty) >> 63) == 0 ||
               (old_net > 0 && signed_qty > 0) ||
               (old_net < 0 && signed_qty < 0)) {
      double old_cost = pos.avg_entry_price * static_cast<double>(old_net);
      double new_cost = trade.price * static_cast<double>(signed_qty);
      pos.avg_entry_price =
          (old_cost + new_cost) / static_cast<double>(pos.net_quantity);
    }
  } else {
    pos.avg_entry_price = 0.0;
  }

  history_[trade.symbol].push_back(
      {trade.timestamp_ns, pos});

  if (callback_) callback_(pos);
}

double PositionTracker::realized_pnl_for_trade(const Trade& trade,
                                                PositionDetail& pos) {
  int64_t signed_qty = side_sign(trade.side) * trade.quantity;
  double pnl = 0.0;

  if (pos.net_quantity == signed_qty) {
    // Opening trade
    pnl = 0.0;
  } else if (pos.net_quantity == 0) {
    // Closing all
    double sign = signed_qty < 0 ? 1.0 : -1.0;
    pnl = sign * static_cast<double>(trade.quantity) *
          (trade.price - pos.avg_entry_price) - trade.fees;
  } else if ((signed_qty > 0 && pos.net_quantity > 0) ||
             (signed_qty < 0 && pos.net_quantity < 0)) {
    // Adding to position
    pnl = -trade.fees;
  } else {
    // Reducing position
    double close_qty =
        static_cast<double>(
            std::min(static_cast<uint64_t>(std::abs(signed_qty)),
                     static_cast<uint64_t>(pos.net_quantity)));
    double sign = pos.net_quantity > 0 ? 1.0 : -1.0;
    pnl = sign * close_qty * (trade.price - pos.avg_entry_price) -
          trade.fees;
  }
  return pnl;
}

PositionDetail PositionTracker::get_position(const std::string& symbol) const {
  std::lock_guard<std::mutex> lock(mu_);
  auto it = positions_.find(symbol);
  if (it != positions_.end()) return it->second;
  return PositionDetail{symbol, 0, 0.0, 0.0, 0.0, 0.0, 0, 0};
}

std::vector<PositionDetail> PositionTracker::get_all_positions() const {
  std::lock_guard<std::mutex> lock(mu_);
  std::vector<PositionDetail> result;
  result.reserve(positions_.size());
  for (auto& [sym, pos] : positions_) result.push_back(pos);
  return result;
}

std::vector<TimestampedPosition> PositionTracker::get_position_history(
    const std::string& symbol) const {
  std::lock_guard<std::mutex> lock(mu_);
  auto it = history_.find(symbol);
  if (it != history_.end()) return it->second;
  return {};
}

void PositionTracker::set_price(const std::string& symbol, double price) {
  std::lock_guard<std::mutex> lock(mu_);
  auto it = positions_.find(symbol);
  if (it != positions_.end()) {
    it->second.current_price = price;
    it->second.update_unrealized();
  }
}

void PositionTracker::set_lot_match_method(LotMatchMethod method) {
  std::lock_guard<std::mutex> lock(mu_);
  lot_method_ = method;
}

void PositionTracker::on_position_update(PositionUpdateCallback cb) {
  std::lock_guard<std::mutex> lock(mu_);
  callback_ = std::move(cb);
}

size_t PositionTracker::symbol_count() const {
  std::lock_guard<std::mutex> lock(mu_);
  return positions_.size();
}

} // namespace risk
