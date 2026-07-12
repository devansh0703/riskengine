#include "risk/trade_journal.h"
#include "risk/timestamp.h"
#include <cstdio>
#include <cstring>
#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
  std::string db_path = "risk_journal.db";
  std::string output = "export.csv";

  for (int i = 1; i < argc; i++) {
    if (std::strcmp(argv[i], "--db") == 0 && i + 1 < argc)
      db_path = argv[++i];
    else if (std::strcmp(argv[i], "--output") == 0 && i + 1 < argc)
      output = argv[++i];
    else if (std::strcmp(argv[i], "--help") == 0) {
      std::printf(
          "Usage: export_tool [--db path] [--output path]\n"
          "Exports trade journal to CSV/Parquet.\n");
      return 0;
    }
  }

  risk::TradeJournal journal(db_path);
  if (!journal.is_open()) {
    std::cerr << "Cannot open " << db_path << "\n";
    return 1;
  }

  std::printf("Exporting %zu trades from %s...\n", journal.trade_count(),
              db_path.c_str());

  if (journal.export_parquet(output)) {
    std::printf("Exported to %s\n", output.c_str());
  } else {
    std::cerr << "Export failed\n";
    return 1;
  }

  return 0;
}
