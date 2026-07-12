#pragma once
#include "types.h"
#include <cstdint>
#include <functional>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace risk {

using AlertCallback = std::function<void(const Alert&)>;

class AlertManager {
 public:
  AlertManager();

  std::vector<Alert> evaluate_rules(int64_t net_qty, double current_price,
                                    double daily_pnl, double drawdown,
                                    double gross_exposure, double net_exposure,
                                    const std::string& symbol);

  void acknowledge(uint64_t alert_id);
  void resolve(uint64_t alert_id, const std::string& notes);
  void escalate(uint64_t alert_id);
  void write_off(uint64_t alert_id, const std::string& notes);

  std::vector<Alert> get_active() const;
  std::vector<Alert> get_history() const;
  std::vector<Alert> get_by_severity(AlertSeverity sev) const;

  void on_alert(AlertCallback cb);
  void set_cooldown_ms(uint64_t ms);

 private:
  mutable std::mutex mu_;
  std::vector<Alert> alerts_;
  std::unordered_map<uint64_t, size_t> id_index_;
  uint64_t next_id_;
  uint64_t cooldown_ms_;
  AlertCallback callback_;

  uint64_t next_alert_id();
  bool in_cooldown(const std::string& key, uint64_t now_ns);
  std::unordered_map<std::string, uint64_t> cooldown_map_;

  Alert create_alert(AlertSeverity sev, const std::string& message);
  void emit_alert(const Alert& a);
};

} // namespace risk
