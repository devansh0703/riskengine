#include "risk/trade_journal.h"
#include "risk/timestamp.h"
#include <cstring>

// SQLite minimal wrapper — forward-declare to avoid header pollution
extern "C" {
#include <sqlite3.h>
}

namespace risk {

TradeJournal::TradeJournal(const std::string& db_path) : db_(nullptr) {
  int rc = sqlite3_open_v2(db_path.c_str(),
                           reinterpret_cast<sqlite3**>(&db_),
                           SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE |
                               SQLITE_OPEN_FULLMUTEX,
                           nullptr);
  if (rc != SQLITE_OK) {
    db_ = nullptr;
    return;
  }
  char* err = nullptr;
  sqlite3_exec(static_cast<sqlite3*>(db_), "PRAGMA journal_mode=WAL;",
               nullptr, nullptr, &err);
  if (err) sqlite3_free(err);
  init_schema();
}

TradeJournal::~TradeJournal() {
  if (db_) sqlite3_close_v2(static_cast<sqlite3*>(db_));
}

void TradeJournal::init_schema() {
  const char* sql =
      "CREATE TABLE IF NOT EXISTS trades ("
      "trade_id INTEGER PRIMARY KEY,"
      "timestamp_ns INTEGER,"
      "symbol TEXT,"
      "side INTEGER,"
      "quantity INTEGER,"
      "price REAL,"
      "fees REAL,"
      "strategy TEXT,"
      "account TEXT,"
      "fill_id TEXT"
      ");"
      "CREATE INDEX IF NOT EXISTS idx_trades_symbol ON trades(symbol);"
      "CREATE INDEX IF NOT EXISTS idx_trades_strategy ON trades(strategy);"
      "CREATE INDEX IF NOT EXISTS idx_trades_timestamp ON trades(timestamp_ns);";
  char* err = nullptr;
  sqlite3_exec(static_cast<sqlite3*>(db_), sql, nullptr, nullptr, &err);
  if (err) sqlite3_free(err);
}

bool TradeJournal::log_trade(const Trade& trade) {
  std::lock_guard<std::mutex> lock(mu_);
  return insert_trade(trade);
}

bool TradeJournal::log_trades(const std::vector<Trade>& trades) {
  std::lock_guard<std::mutex> lock(mu_);
  auto* db = static_cast<sqlite3*>(db_);
  sqlite3_exec(db, "BEGIN;", nullptr, nullptr, nullptr);
  for (auto& t : trades) {
    if (!insert_trade(t)) {
      sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
      return false;
    }
  }
  sqlite3_exec(db, "COMMIT;", nullptr, nullptr, nullptr);
  return true;
}

bool TradeJournal::insert_trade(const Trade& trade) {
  auto* db = static_cast<sqlite3*>(db_);
  sqlite3_stmt* stmt = nullptr;
  const char* sql =
      "INSERT INTO trades VALUES (?,?,?,?,?,?,?,?,?,?)";
  int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
  if (rc != SQLITE_OK) return false;

  sqlite3_bind_int64(stmt, 1, static_cast<sqlite3_int64>(trade.trade_id));
  sqlite3_bind_int64(stmt, 2, static_cast<sqlite3_int64>(trade.timestamp_ns));
  sqlite3_bind_text(stmt, 3, trade.symbol.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_int(stmt, 4, static_cast<int>(trade.side));
  sqlite3_bind_int64(stmt, 5, trade.quantity);
  sqlite3_bind_double(stmt, 6, trade.price);
  sqlite3_bind_double(stmt, 7, trade.fees);
  sqlite3_bind_text(stmt, 8, trade.strategy.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 9, trade.account.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 10, trade.fill_id.c_str(), -1, SQLITE_TRANSIENT);

  rc = sqlite3_step(stmt);
  sqlite3_finalize(stmt);
  return rc == SQLITE_DONE;
}

std::vector<TradeRecord> TradeJournal::query(
    const DateRange& range, const std::string& symbol,
    const std::string& strategy, const std::string& account) const {
  std::lock_guard<std::mutex> lock(mu_);
  std::vector<TradeRecord> results;
  auto* db = static_cast<sqlite3*>(db_);

  std::string sql =
      "SELECT trade_id,timestamp_ns,symbol,side,quantity,price,fees,"
      "strategy,account,fill_id FROM trades WHERE timestamp_ns>=? AND "
      "timestamp_ns<=?";
  std::vector<std::string> params;
  params.push_back(std::to_string(range.start_ns));
  params.push_back(std::to_string(range.end_ns));

  if (!symbol.empty()) {
    sql += " AND symbol=?";
    params.push_back(symbol);
  }
  if (!strategy.empty()) {
    sql += " AND strategy=?";
    params.push_back(strategy);
  }
  if (!account.empty()) {
    sql += " AND account=?";
    params.push_back(account);
  }

  sql += " ORDER BY timestamp_ns";

  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK)
    return results;

  int idx = 1;
  for (auto& p : params) {
    sqlite3_bind_text(stmt, idx++, p.c_str(), -1, SQLITE_TRANSIENT);
  }

  while (sqlite3_step(stmt) == SQLITE_ROW) {
    TradeRecord rec{};
    rec.trade_id = static_cast<uint64_t>(sqlite3_column_int64(stmt, 0));
    rec.timestamp_ns = static_cast<uint64_t>(sqlite3_column_int64(stmt, 1));
    rec.symbol =
        reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
    rec.side = static_cast<Side>(sqlite3_column_int(stmt, 3));
    rec.quantity = sqlite3_column_int64(stmt, 4);
    rec.price = sqlite3_column_double(stmt, 5);
    rec.fees = sqlite3_column_double(stmt, 6);
    rec.strategy =
        reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));
    rec.account =
        reinterpret_cast<const char*>(sqlite3_column_text(stmt, 8));
    rec.fill_id =
        reinterpret_cast<const char*>(sqlite3_column_text(stmt, 9));
    results.push_back(rec);
  }
  sqlite3_finalize(stmt);
  return results;
}

size_t TradeJournal::trade_count() const {
  std::lock_guard<std::mutex> lock(mu_);
  auto* db = static_cast<sqlite3*>(db_);
  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(db, "SELECT COUNT(*) FROM trades", -1, &stmt,
                         nullptr) != SQLITE_OK)
    return 0;
  size_t count = 0;
  if (sqlite3_step(stmt) == SQLITE_ROW)
    count = static_cast<size_t>(sqlite3_column_int64(stmt, 0));
  sqlite3_finalize(stmt);
  return count;
}

bool TradeJournal::export_parquet(const std::string& output_path) const {
  // Export to CSV as Parquet requires arrow library
  std::lock_guard<std::mutex> lock(mu_);
  auto* db = static_cast<sqlite3*>(db_);
  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(db,
                         "SELECT trade_id,timestamp_ns,symbol,side,quantity,"
                         "price,fees,strategy,account,fill_id FROM trades "
                         "ORDER BY timestamp_ns",
                         -1, &stmt, nullptr) != SQLITE_OK)
    return false;

  std::string csv_path = output_path;
  if (csv_path.size() >= 8 &&
      csv_path.substr(csv_path.size() - 8) == ".parquet") {
    csv_path = csv_path.substr(0, csv_path.size() - 8) + ".csv";
  }

  FILE* f = fopen(csv_path.c_str(), "w");
  if (!f) {
    sqlite3_finalize(stmt);
    return false;
  }
  fprintf(f,
          "trade_id,timestamp_ns,symbol,side,quantity,price,fees,strategy,"
          "account,fill_id\n");
  while (sqlite3_step(stmt) == SQLITE_ROW) {
    fprintf(f, "%lld,%lld,%s,%d,%lld,%.6f,%.6f,%s,%s,%s\n",
            sqlite3_column_int64(stmt, 0), sqlite3_column_int64(stmt, 1),
            sqlite3_column_text(stmt, 2), sqlite3_column_int(stmt, 3),
            sqlite3_column_int64(stmt, 4), sqlite3_column_double(stmt, 5),
            sqlite3_column_double(stmt, 6), sqlite3_column_text(stmt, 7),
            sqlite3_column_text(stmt, 8), sqlite3_column_text(stmt, 9));
  }
  fclose(f);
  sqlite3_finalize(stmt);
  return true;
}

} // namespace risk
