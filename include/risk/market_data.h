#pragma once
#include <cstdint>
#include <functional>
#include <mutex>
#include <string>
#include <unordered_map>

namespace risk {

struct PriceTick {
  uint64_t timestamp_ns;
  std::string symbol;
  double bid;
  double ask;
  double last;
  double mid() const { return (bid + ask) / 2.0; }
  double spread() const { return ask - bid; }
};

using PriceCallback = std::function<void(const PriceTick&)>;

class MarketData {
 public:
  MarketData();
  ~MarketData();

  void update_price(const PriceTick& tick);
  PriceTick get_price(const std::string& symbol) const;
  bool has_price(const std::string& symbol) const;
  bool is_stale(const std::string& symbol, uint64_t max_age_ns) const;

  void on_price_update(PriceCallback cb);
  size_t symbol_count() const;

  void set_market_hours(int open_hour_utc, int close_hour_utc);
  bool is_market_open(uint64_t timestamp_ns) const;

  void load_from_csv(const std::string& path);
  bool replay_next(PriceTick* out);
  void reset_replay();
  size_t replay_remaining() const;

 private:
  mutable std::mutex mu_;
  std::unordered_map<std::string, PriceTick> cache_;
  PriceCallback callback_;
  int open_hour_;
  int close_hour_;

  std::vector<PriceTick> replay_data_;
  size_t replay_index_;
};

} // namespace risk
