#include "risk/position_tracker.h"
#include "risk/pnl_calculator.h"
#include "risk/trade_journal.h"
#include "risk/timestamp.h"
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

static risk::Trade parse_trade_line(const std::string& line) {
  std::istringstream ss(line);
  std::string token;
  risk::Trade t{};
  std::getline(ss, token, ',');
  t.trade_id = std::stoull(token);
  std::getline(ss, token, ',');
  t.timestamp_ns = std::stoull(token);
  std::getline(ss, t.symbol, ',');
  std::getline(ss, token, ',');
  t.side = token == "Buy" ? risk::Side::Buy : risk::Side::Sell;
  std::getline(ss, token, ',');
  t.quantity = std::stoll(token);
  std::getline(ss, token, ',');
  t.price = std::stod(token);
  std::getline(ss, token, ',');
  t.fees = std::stod(token);
  std::getline(ss, t.strategy, ',');
  std::getline(ss, t.account, ',');
  std::getline(ss, t.fill_id, ',');
  return t;
}

int main(int argc, char* argv[]) {
  if (argc < 2) {
    std::cerr << "Usage: replay_tool <csv_path> [--db path]\n";
    return 1;
  }

  std::string csv_path = argv[1];
  std::string db_path = "risk_journal.db";

  for (int i = 2; i < argc; i++) {
    if (std::strcmp(argv[i], "--db") == 0 && i + 1 < argc)
      db_path = argv[++i];
  }

  risk::PositionTracker tracker;
  risk::PnLCalculator pnl;
  risk::TradeJournal journal(db_path);

  std::ifstream file(csv_path);
  if (!file.is_open()) {
    std::cerr << "Cannot open " << csv_path << "\n";
    return 1;
  }

  std::string line;
  std::getline(file, line); // skip header
  size_t count = 0;
  double last_price = 0;

  while (std::getline(file, line)) {
    if (line.empty()) continue;
    auto trade = parse_trade_line(line);
    last_price = trade.price;

    tracker.update_position(trade);
    pnl.record_trade(trade, 0);
    journal.log_trade(trade);
    count++;

    if (count % 1000 == 0) {
      std::printf("\r  Processed %zu trades...", count);
      std::fflush(stdout);
    }
  }

  std::printf("\nReplay complete: %zu trades\n", count);
  auto positions = tracker.get_all_positions();
  std::printf("Open positions: %zu symbols\n", positions.size());

  for (auto& p : positions) {
    std::printf("  %s: %ld @ $%.2f (realized: $%.2f)\n",
                p.symbol.c_str(), (long)p.net_quantity, p.avg_entry_price,
                p.realized_pnl);
  }

  std::printf("Total realized PnL: $%.2f\n", pnl.cumulative_realized());
  std::printf("Total fees: $%.2f\n", pnl.total_fees());
  return 0;
}
