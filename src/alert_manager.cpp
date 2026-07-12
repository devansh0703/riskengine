#include "risk/alert_manager.h"
#include "risk/timestamp.h"
#include <algorithm>
#include <sstream>

namespace risk {

AlertManager::AlertManager()
    : next_id_(1), cooldown_ms_(60000), callback_(nullptr) {}

uint64_t AlertManager::next_alert_id() { return next_id_++; }

bool AlertManager::in_cooldown(const std::string& key, uint64_t now_ns) {
  auto it = cooldown_map_.find(key);
  if (it == cooldown_map_.end()) return false;
  uint64_t elapsed_ms = (now_ns - it->second) / 1000000;
  return elapsed_ms < cooldown_ms_;
}

Alert AlertManager::create_alert(AlertSeverity sev,
                                 const std::string& message) {
  uint64_t now = now_ns();
  Alert a{};
  a.id = next_alert_id();
  a.severity = sev;
  a.state = AlertState::Pending;
  a.message = message;
  a.created_at_ns = now;
  a.updated_at_ns = now;
  return a;
}

void AlertManager::emit_alert(const Alert& a) {
  if (callback_) callback_(a);
}

std::vector<Alert> AlertManager::evaluate_rules(
    int64_t net_qty, double current_price, double daily_pnl,
    double drawdown, double gross_exposure, double net_exposure,
    const std::string& symbol) {
  std::lock_guard<std::mutex> lock(mu_);
  std::vector<Alert> new_alerts;
  uint64_t now = now_ns();

  auto try_alert = [&](AlertSeverity sev, const std::string& key,
                       const std::string& msg) {
    if (!in_cooldown(key, now)) {
      Alert a = create_alert(sev, msg);
      a.state = AlertState::Active;
      id_index_[a.id] = alerts_.size();
      alerts_.push_back(a);
      cooldown_map_[key] = now;
      new_alerts.push_back(a);
      emit_alert(a);
    }
  };

  double mkt_val = static_cast<double>(net_qty) * current_price;
  if (std::abs(mkt_val) > 1e7) {
    try_alert(AlertSeverity::Warning, "pos_" + symbol,
              "Large position in " + symbol + ": " +
                  std::to_string(static_cast<int64_t>(net_qty)) + " shares");
  }

  if (daily_pnl < -500000) {
    try_alert(AlertSeverity::Critical, "daily_loss",
              "Daily loss exceeds $500K: " + std::to_string(daily_pnl));
  } else if (daily_pnl < -200000) {
    try_alert(AlertSeverity::Warning, "daily_loss_warn",
              "Daily loss approaching limit: " + std::to_string(daily_pnl));
  }

  if (drawdown > 1000000) {
    try_alert(AlertSeverity::Critical, "drawdown",
              "Drawdown exceeds $1M: " + std::to_string(drawdown));
  }

  if (gross_exposure > 5e8) {
    try_alert(AlertSeverity::Warning, "gross_exp",
              "Gross exposure exceeds $500M");
  }

  return new_alerts;
}

void AlertManager::acknowledge(uint64_t alert_id) {
  std::lock_guard<std::mutex> lock(mu_);
  auto it = id_index_.find(alert_id);
  if (it != id_index_.end()) {
    alerts_[it->second].state = AlertState::Acknowledged;
    alerts_[it->second].updated_at_ns = now_ns();
  }
}

void AlertManager::resolve(uint64_t alert_id, const std::string& notes) {
  std::lock_guard<std::mutex> lock(mu_);
  auto it = id_index_.find(alert_id);
  if (it != id_index_.end()) {
    alerts_[it->second].state = AlertState::Resolved;
    alerts_[it->second].notes = notes;
    alerts_[it->second].updated_at_ns = now_ns();
  }
}

void AlertManager::escalate(uint64_t alert_id) {
  std::lock_guard<std::mutex> lock(mu_);
  auto it = id_index_.find(alert_id);
  if (it != id_index_.end()) {
    alerts_[it->second].state = AlertState::Escalated;
    alerts_[it->second].updated_at_ns = now_ns();
    emit_alert(alerts_[it->second]);
  }
}

void AlertManager::write_off(uint64_t alert_id, const std::string& notes) {
  std::lock_guard<std::mutex> lock(mu_);
  auto it = id_index_.find(alert_id);
  if (it != id_index_.end()) {
    alerts_[it->second].state = AlertState::WrittenOff;
    alerts_[it->second].notes = notes;
    alerts_[it->second].updated_at_ns = now_ns();
  }
}

std::vector<Alert> AlertManager::get_active() const {
  std::lock_guard<std::mutex> lock(mu_);
  std::vector<Alert> result;
  for (auto& a : alerts_)
    if (a.state == AlertState::Active || a.state == AlertState::Pending)
      result.push_back(a);
  return result;
}

std::vector<Alert> AlertManager::get_history() const {
  std::lock_guard<std::mutex> lock(mu_);
  return alerts_;
}

std::vector<Alert> AlertManager::get_by_severity(AlertSeverity sev) const {
  std::lock_guard<std::mutex> lock(mu_);
  std::vector<Alert> result;
  for (auto& a : alerts_)
    if (a.severity == sev) result.push_back(a);
  return result;
}

void AlertManager::on_alert(AlertCallback cb) {
  std::lock_guard<std::mutex> lock(mu_);
  callback_ = std::move(cb);
}

void AlertManager::set_cooldown_ms(uint64_t ms) {
  std::lock_guard<std::mutex> lock(mu_);
  cooldown_ms_ = ms;
}

} // namespace risk
