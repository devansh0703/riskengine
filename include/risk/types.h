#pragma once
#include <cstdint>
#include <string>

namespace risk {

enum class Side : uint8_t { Buy = 0, Sell = 1 };
enum class OrderType : uint8_t { Limit = 0, Market = 1 };
enum class AlertSeverity : uint8_t { Info = 0, Warning = 1, Critical = 2 };
enum class AlertState : uint8_t {
  Pending = 0,
  Active = 1,
  Acknowledged = 2,
  Resolved = 3,
  Escalated = 4,
  WrittenOff = 5
};
enum class LotMatchMethod : uint8_t { FIFO = 0, LIFO = 1, AverageCost = 2 };
enum class SettlementStatus : uint8_t {
  Pending = 0,
  Instructed = 1,
  Settled = 2,
  Failed = 3,
  BuyIn = 4
};

inline int64_t side_sign(Side s) { return s == Side::Buy ? 1 : -1; }

struct Trade {
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

struct Position {
  std::string symbol;
  int64_t net_quantity;
  double avg_entry_price;
  double realized_pnl;
  double unrealized_pnl;
  double current_price;
};

struct Alert {
  uint64_t id;
  AlertSeverity severity;
  AlertState state;
  std::string message;
  uint64_t created_at_ns;
  uint64_t updated_at_ns;
  std::string assignee;
  std::string notes;
};

} // namespace risk
