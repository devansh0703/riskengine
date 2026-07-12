#pragma once
#include "types.h"
#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

namespace risk {

struct TradeRecord {
  uint64_t trade_id;
  uint64_t timestamp_ns;
  std::string symbol;
  Side side;
  int64_t quantity;
  double price;
  double fees;
  std::string strategy;
  std::string account;
  std::string fill_id;
};

struct DateRange {
  uint64_t start_ns;
  uint64_t end_ns;
};

class TradeJournal {
 public:
  explicit TradeJournal(const std::string& db_path = ":memory:");
  ~TradeJournal();

  bool log_trade(const Trade& trade);
  bool log_trades(const std::vector<Trade>& trades);

  std::vector<TradeRecord> query(const DateRange& range,
                                 const std::string& symbol = "",
                                 const std::string& strategy = "",
                                 const std::string& account = "") const;

  size_t trade_count() const;
  bool export_parquet(const std::string& output_path) const;

  bool is_open() const { return db_ != nullptr; }

 private:
  mutable std::mutex mu_;
  void* db_; // sqlite3*

  void init_schema();
  bool insert_trade(const Trade& trade);
};

} // namespace risk
