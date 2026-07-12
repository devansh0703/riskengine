#include "risk/market_data.h"
#include <fstream>
#include <sstream>
#include <algorithm>

namespace risk {

MarketData::MarketData()
    : callback_(nullptr), open_hour_(9), close_hour_(16),
      replay_index_(0) {}

MarketData::~MarketData() = default;

void MarketData::update_price(const PriceTick& tick) {
  std::lock_guard<std::mutex> lock(mu_);
  cache_[tick.symbol] = tick;
  if (callback_) callback_(tick);
}

PriceTick MarketData::get_price(const std::string& symbol) const {
  std::lock_guard<std::mutex> lock(mu_);
  auto it = cache_.find(symbol);
  if (it != cache_.end()) return it->second;
  return PriceTick{0, symbol, 0, 0, 0};
}

bool MarketData::has_price(const std::string& symbol) const {
  std::lock_guard<std::mutex> lock(mu_);
  return cache_.count(symbol) > 0;
}

bool MarketData::is_stale(const std::string& symbol,
                          uint64_t max_age_ns) const {
  std::lock_guard<std::mutex> lock(mu_);
  auto it = cache_.find(symbol);
  if (it == cache_.end()) return true;
  uint64_t now = static_cast<uint64_t>(
      std::chrono::duration_cast<std::chrono::nanoseconds>(
          std::chrono::system_clock::now().time_since_epoch())
          .count());
  return (now - it->second.timestamp_ns) > max_age_ns;
}

void MarketData::on_price_update(PriceCallback cb) {
  std::lock_guard<std::mutex> lock(mu_);
  callback_ = std::move(cb);
}

size_t MarketData::symbol_count() const {
  std::lock_guard<std::mutex> lock(mu_);
  return cache_.size();
}

void MarketData::set_market_hours(int open_hour_utc, int close_hour_utc) {
  std::lock_guard<std::mutex> lock(mu_);
  open_hour_ = open_hour_utc;
  close_hour_ = close_hour_utc;
}

bool MarketData::is_market_open(uint64_t timestamp_ns) const {
  auto tp = std::chrono::system_clock::time_point(
      std::chrono::nanoseconds(timestamp_ns));
  auto tt = std::chrono::system_clock::to_time_t(tp);
  std::tm* tm = std::gmtime(&tt);
  int hour = tm->tm_hour;
  return hour >= open_hour_ && hour < close_hour_;
}

void MarketData::load_from_csv(const std::string& path) {
  std::lock_guard<std::mutex> lock(mu_);
  replay_data_.clear();
  replay_index_ = 0;

  std::ifstream file(path);
  if (!file.is_open()) return;

  std::string line;
  std::getline(file, line); // skip header

  while (std::getline(file, line)) {
    std::istringstream ss(line);
    std::string token;
    PriceTick tick{};

    std::getline(ss, token, ',');
    tick.timestamp_ns = std::stoull(token);
    std::getline(ss, tick.symbol, ',');
    std::getline(ss, token, ',');
    tick.bid = std::stod(token);
    std::getline(ss, token, ',');
    tick.ask = std::stod(token);
    std::getline(ss, token, ',');
    tick.last = std::stod(token);

    replay_data_.push_back(tick);
  }

  std::sort(replay_data_.begin(), replay_data_.end(),
            [](const PriceTick& a, const PriceTick& b) {
              return a.timestamp_ns < b.timestamp_ns;
            });
}

bool MarketData::replay_next(PriceTick* out) {
  std::lock_guard<std::mutex> lock(mu_);
  if (replay_index_ >= replay_data_.size()) return false;
  *out = replay_data_[replay_index_++];
  cache_[out->symbol] = *out;
  return true;
}

void MarketData::reset_replay() {
  std::lock_guard<std::mutex> lock(mu_);
  replay_index_ = 0;
}

size_t MarketData::replay_remaining() const {
  std::lock_guard<std::mutex> lock(mu_);
  if (replay_index_ >= replay_data_.size()) return 0;
  return replay_data_.size() - replay_index_;
}

} // namespace risk
