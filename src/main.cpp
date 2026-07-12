#include "risk/alert_manager.h"
#include "risk/lot_matching.h"
#include "risk/market_data.h"
#include "risk/metrics_engine.h"
#include "risk/pnl_calculator.h"
#include "risk/position_tracker.h"
#include "risk/risk_limits.h"
#include "risk/timestamp.h"
#include "risk/trade_journal.h"
#include <csignal>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

static volatile bool g_running = true;

static void signal_handler(int) { g_running = false; }

static void print_usage(const char* prog) {
  std::cerr << "Usage: " << prog
            << " [--mode live|replay|status] [--config path] [--csv path]\n";
}

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

static void run_replay(const std::string& csv_path,
                       const std::string& db_path) {
  risk::PositionTracker tracker;
  risk::PnLCalculator pnl;
  risk::LotMatching lots(risk::LotMatchMethod::FIFO);
  risk::TradeJournal journal(db_path);
  risk::MarketData market;
  risk::MetricsEngine metrics;
  risk::RiskLimits limits;
  risk::AlertManager alerts;

  limits.set_daily_stop_loss(500000);
  limits.set_max_drawdown(1000000);

  std::ifstream file(csv_path);
  if (!file.is_open()) {
    std::cerr << "Cannot open " << csv_path << "\n";
    return;
  }

  std::string line;
  std::getline(file, line); // header
  size_t count = 0;

  while (g_running && std::getline(file, line)) {
    if (line.empty()) continue;
    auto trade = parse_trade_line(line);

    tracker.update_position(trade);
    pnl.record_trade(trade, 0);

    auto pos = tracker.get_position(trade.symbol);
    pnl.update_unrealized(trade.symbol, pos.net_quantity,
                          pos.avg_entry_price, trade.price);

    auto exp = metrics.compute_exposure();
    auto breaches = limits.check_position(
        pos.net_quantity, trade.price, trade.symbol);
    if (limits.has_hard_breach(breaches)) {
      std::cerr << "HARD BREACH on " << trade.symbol << "\n";
    }

    journal.log_trade(trade);
    count++;

    if (count % 1000 == 0) {
      std::cerr << "Processed " << count << " trades, PnL: $"
                << pnl.total_pnl() << "\n";
    }
  }

  std::cout << "Replay complete. " << count << " trades processed.\n";
  std::cout << "Total realized PnL: $" << pnl.cumulative_realized() << "\n";
  std::cout << "Total fees: $" << pnl.total_fees() << "\n";
  std::cout << "Journal entries: " << journal.trade_count() << "\n";
}

static void run_status(const std::string& db_path) {
  risk::TradeJournal journal(db_path);
  if (!journal.is_open()) {
    std::cerr << "Cannot open journal at " << db_path << "\n";
    return;
  }
  std::cout << "Trade journal: " << journal.trade_count() << " trades\n";

  auto now = risk::now_ns();
  auto day_ago = now - 86400000000000ULL;
  risk::DateRange range{day_ago, now};
  auto trades = journal.query(range);
  std::cout << "Last 24h: " << trades.size() << " trades\n";

  double total_pnl = 0;
  double total_fees = 0;
  for (auto& t : trades) {
    total_fees += t.fees;
  }
  std::cout << "Fees last 24h: $" << total_fees << "\n";
}

int main(int argc, char* argv[]) {
  std::signal(SIGINT, signal_handler);
  std::signal(SIGTERM, signal_handler);

  std::string mode = "replay";
  std::string config = "config/default.yaml";
  std::string csv_path = "data/sample_trades.csv";
  std::string db_path = "risk_journal.db";

  for (int i = 1; i < argc; i++) {
    if (std::strcmp(argv[i], "--mode") == 0 && i + 1 < argc)
      mode = argv[++i];
    else if (std::strcmp(argv[i], "--config") == 0 && i + 1 < argc)
      config = argv[++i];
    else if (std::strcmp(argv[i], "--csv") == 0 && i + 1 < argc)
      csv_path = argv[++i];
    else if (std::strcmp(argv[i], "--db") == 0 && i + 1 < argc)
      db_path = argv[++i];
    else if (std::strcmp(argv[i], "--help") == 0) {
      print_usage(argv[0]);
      return 0;
    }
  }

  if (mode == "replay") {
    run_replay(csv_path, db_path);
  } else if (mode == "status") {
    run_status(db_path);
  } else {
    std::cerr << "Unknown mode: " << mode << "\n";
    print_usage(argv[0]);
    return 1;
  }

  return 0;
}
