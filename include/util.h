#pragma once

#include <chrono>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

#include "types.h"

#ifndef DEBUG_LEVEL
#define DEBUG_LEVEL LogLevel::Debug
#endif

#define PropertyWithGetter(type, var) \
 private:                             \
  type var##_;                        \
                                      \
 public:                              \
  const type &var() const { return var##_; }

namespace bolo {
inline Timestamp GetTimestamp() {
  using namespace std::chrono;
  auto now = system_clock::now();
  return static_cast<Timestamp>(duration_cast<microseconds>(now.time_since_epoch()).count());
}

inline std::string TimestampToString(Timestamp stamp) {
  namespace ch = std::chrono;

  ch::microseconds s{static_cast<ch::microseconds>(stamp)};
  ch::system_clock::time_point p{ch::duration{s}};

  auto t = ch::system_clock::to_time_t(p);

  std::stringstream ss;
  ss << std::put_time(std::localtime(&t), "%F %T");
  return ss.str();
}

inline std::string MakeTemp() {
  using namespace std::string_literals;
  static int temp_id = 0;
  auto path = std::filesystem::temp_directory_path() / ("bolo_num_"s + std::to_string(temp_id++));
  if (std::filesystem::exists(path)) std::filesystem::remove(path);
  return path.string();
}

enum class LogLevel : uint8_t {
  None,
  Info,
  Warning,
  Error,
  Debug,
};

inline void Log(LogLevel level, const std::string &s) {
  if (level <= DEBUG_LEVEL) std::cerr << s << "\n";
}

};  // namespace bolo
