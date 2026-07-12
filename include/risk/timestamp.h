#pragma once
#include <chrono>
#include <cstdint>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <string>

namespace risk {

using Clock = std::chrono::system_clock;
using TimePoint = Clock::time_point;
using Duration = Clock::duration;

inline uint64_t now_ns() {
  return static_cast<uint64_t>(
      std::chrono::duration_cast<std::chrono::nanoseconds>(
          Clock::now().time_since_epoch())
          .count());
}

inline TimePoint ns_to_tp(uint64_t ns) {
  return TimePoint(Duration(ns));
}

inline uint64_t tp_to_ns(TimePoint tp) {
  return static_cast<uint64_t>(
      std::chrono::duration_cast<std::chrono::nanoseconds>(
          tp.time_since_epoch())
          .count());
}

inline std::string ns_to_string(uint64_t ns) {
  auto tp = ns_to_tp(ns);
  auto tt = Clock::to_time_t(tp);
  auto ms = (ns / 1000000) % 1000;
  std::ostringstream oss;
  oss << std::put_time(std::gmtime(&tt), "%Y-%m-%dT%H:%M:%S")
      << '.' << std::setfill('0') << std::setw(3) << ms << "Z";
  return oss.str();
}

inline uint64_t date_cutoff_ns(uint64_t reference_ns, int hour_utc) {
  auto tp = ns_to_tp(reference_ns);
  auto tt = Clock::to_time_t(tp);
  std::tm tm = *std::gmtime(&tt);
  tm.tm_hour = hour_utc;
  tm.tm_min = 0;
  tm.tm_sec = 0;
  auto cutoff_tp = Clock::from_time_t(std::mktime(&tm));
  if (cutoff_tp > tp) cutoff_tp -= std::chrono::hours(24);
  return tp_to_ns(cutoff_tp);
}

} // namespace risk
