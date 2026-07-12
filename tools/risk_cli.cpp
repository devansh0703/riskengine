#include "risk/alert_manager.h"
#include "risk/market_data.h"
#include "risk/metrics_engine.h"
#include "risk/pnl_calculator.h"
#include "risk/position_tracker.h"
#include "risk/risk_limits.h"
#include "risk/trade_journal.h"
#include "risk/timestamp.h"
#include <cstdio>
#include <cstring>
#include <iostream>
#include <string>

static void cmd_status(const std::string& db_path) {
  risk::TradeJournal journal(db_path);
  if (!journal.is_open()) {
    std::cerr << "Cannot open " << db_path << "\n";
    return;
  }
  std::printf("Trade Journal: %zu trades\n", journal.trade_count());

  auto now = risk::now_ns();
  auto day_ago = now - 86400000000000ULL;
  auto trades = journal.query({day_ago, now});
  std::printf("Last 24h: %zu trades\n", trades.size());

  double total_fees = 0;
  for (auto& t : trades) total_fees += t.fees;
  std::printf("Fees 24h: $%.2f\n", total_fees);
}

static void cmd_query(const std::string& db_path, const std::string& symbol,
                      size_t limit) {
  risk::TradeJournal journal(db_path);
  if (!journal.is_open()) {
    std::cerr << "Cannot open " << db_path << "\n";
    return;
  }
  auto now = risk::now_ns();
  auto week_ago = now - 7 * 86400000000000ULL;
  auto trades = journal.query({week_ago, now}, symbol);

  size_t shown = 0;
  for (auto& t : trades) {
    if (shown >= limit) break;
    std::printf("[%lu] %s %s %ld @ $%.2f (fees: $%.2f)\n",
                (unsigned long)t.trade_id, t.symbol.c_str(),
                t.side == risk::Side::Buy ? "BUY" : "SELL",
                (long)t.quantity, t.price, t.fees);
    shown++;
  }
  std::printf("Showing %zu of %zu trades\n", shown, trades.size());
}

static void cmd_export(const std::string& db_path,
                       const std::string& output) {
  risk::TradeJournal journal(db_path);
  if (!journal.is_open()) {
    std::cerr << "Cannot open " << db_path << "\n";
    return;
  }
  if (journal.export_parquet(output)) {
    std::printf("Exported to %s\n", output.c_str());
  } else {
    std::cerr << "Export failed\n";
  }
}

int main(int argc, char* argv[]) {
  if (argc < 2) {
    std::cerr << "Usage: risk_cli <status|query|export> [options]\n";
    return 1;
  }

  std::string cmd = argv[1];
  std::string db = "risk_journal.db";
  std::string symbol;
  std::string output = "export.csv";
  size_t limit = 50;

  for (int i = 2; i < argc; i++) {
    if (std::strcmp(argv[i], "--db") == 0 && i + 1 < argc)
      db = argv[++i];
    else if (std::strcmp(argv[i], "--symbol") == 0 && i + 1 < argc)
      symbol = argv[++i];
    else if (std::strcmp(argv[i], "--output") == 0 && i + 1 < argc)
      output = argv[++i];
    else if (std::strcmp(argv[i], "--limit") == 0 && i + 1 < argc)
      limit = std::stoul(argv[++i]);
  }

  if (cmd == "status")
    cmd_status(db);
  else if (cmd == "query")
    cmd_query(db, symbol, limit);
  else if (cmd == "export")
    cmd_export(db, output);
  else
    std::cerr << "Unknown command: " << cmd << "\n";

  return 0;
}
