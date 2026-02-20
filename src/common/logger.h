#pragma once
/**
 * @file logger.h
 * @brief Lightweight, thread-safe logging utility for KVMemo.
 *
 * KVMemo is a concurrent TCP-based key-value store. Logging must therefore:
 *  - Be thread-safe
 *  - Have minimal overhead
 *  - Avoid external dependencies
 *  - Support runtime log-level filtering
 *  - Work cross-platform (Linux/macOS/Windows)
 * 
 * Logging Format:
 *   [EPOCH_MS] [LEVEL] [tid=THREAD_ID] file:line | message
 *
 * Example:
 *   [1700000123456] [INFO ] [tid=1234] server.cpp:42 | Server started
 *
 * Production Note:
 *   - FATAL logs terminate the process using std::abort().
 *   - Default log level is INFO.
 *
 * Copyright (c) 2026 Gagan
 * All rights reserved.
 */

#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>

#include "src/common/time.h"

namespace kvmemo::common {

/**
 * @enum LogLevel
 * @brief Defines severity levels for logging.
 *
 * Ordered by increasing severity.
 */
enum class LogLevel : std::uint8_t {
  kTrace = 0,
  kDebug = 1,
  kInfo  = 2,
  kWarn  = 3,
  kError = 4,
  kFatal = 5,
  kOff   = 6
};

/**
 * @class Logger
 * @brief Static logging utility.
 *
 * This class is intentionally non-instantiable.
 * All methods are static.
 *
 * Thread Safety:
 *  - Log writes are guarded by a mutex.
 *  - Log level stored in atomic variable.
 *
 * Performance:
 *  - Log-level check happens before formatting.
 *  - Avoid heavy formatting when disabled.
 */
class Logger final {
 public:
  Logger() = delete;

  /**
   * @brief Sets global log level.
   */
  static void SetLevel(LogLevel level) noexcept {
    level_.store(level, std::memory_order_relaxed);
  }

  /**
   * @brief Returns current global log level.
   */
  [[nodiscard]] static LogLevel GetLevel() noexcept {
    return level_.load(std::memory_order_relaxed);
  }

  /**
   * @brief Checks if given level should be logged.
   */
  [[nodiscard]] static bool IsEnabled(LogLevel level) noexcept {
    LogLevel current = GetLevel();
    if (current == LogLevel::kOff) {
      return false;
    }
    return level >= current;
  }

  /**
   * @brief Core logging function.
   *
   * @param level Log severity
   * @param file Source file
   * @param line Source line number
   * @param message Preformatted log message
   */
  static void Log(LogLevel level,
                  const char* file,
                  int line,
                  const std::string& message) noexcept {
    if (!IsEnabled(level)) {
      return;
    }

    std::ostringstream oss;
    AppendHeader(oss, level);
    oss << " " << file << ":" << line << " | " << message << "\n";

    const std::string output = oss.str();

    {
      std::lock_guard<std::mutex> lock(mu_);
      std::cerr << output;
      std::cerr.flush();
    }

    if (level == LogLevel::kFatal) {
      std::abort();
    }
  }

 private:
  /**
   * @brief Appends formatted header.
   */
  static void AppendHeader(std::ostringstream& oss,
                           LogLevel level) noexcept {
    const EpochMillis ts = Clock::NowEpochMillis();
    oss << "[" << ts << "] ";
    oss << "[" << LevelToString(level) << "] ";
    oss << "[tid=" << std::this_thread::get_id() << "]";
  }

  /**
   * @brief Converts level to readable string.
   */
  [[nodiscard]] static const char* LevelToString(LogLevel level) noexcept {
    switch (level) {
      case LogLevel::kTrace: return "TRACE";
      case LogLevel::kDebug: return "DEBUG";
      case LogLevel::kInfo:  return "INFO ";
      case LogLevel::kWarn:  return "WARN ";
      case LogLevel::kError: return "ERROR";
      case LogLevel::kFatal: return "FATAL";
      case LogLevel::kOff:   return "OFF  ";
      default:               return "UNKWN";
    }
  }

 private:
  inline static std::atomic<LogLevel> level_{LogLevel::kInfo};
  inline static std::mutex mu_;
};

}  // namespace kvmemo::common


// =============================
// Logging Macros
// =============================

#define KV_LOG_TRACE(msg) \
  do { \
    if (::kvmemo::common::Logger::IsEnabled(::kvmemo::common::LogLevel::kTrace)) \
      ::kvmemo::common::Logger::Log(::kvmemo::common::LogLevel::kTrace, __FILE__, __LINE__, (msg)); \
  } while (0)

#define KV_LOG_DEBUG(msg) \
  do { \
    if (::kvmemo::common::Logger::IsEnabled(::kvmemo::common::LogLevel::kDebug)) \
      ::kvmemo::common::Logger::Log(::kvmemo::common::LogLevel::kDebug, __FILE__, __LINE__, (msg)); \
  } while (0)

#define KV_LOG_INFO(msg) \
  do { \
    if (::kvmemo::common::Logger::IsEnabled(::kvmemo::common::LogLevel::kInfo)) \
      ::kvmemo::common::Logger::Log(::kvmemo::common::LogLevel::kInfo, __FILE__, __LINE__, (msg)); \
  } while (0)

#define KV_LOG_WARN(msg) \
  do { \
    if (::kvmemo::common::Logger::IsEnabled(::kvmemo::common::LogLevel::kWarn)) \
      ::kvmemo::common::Logger::Log(::kvmemo::common::LogLevel::kWarn, __FILE__, __LINE__, (msg)); \
  } while (0)

#define KV_LOG_ERROR(msg) \
  do { \
    if (::kvmemo::common::Logger::IsEnabled(::kvmemo::common::LogLevel::kError)) \
      ::kvmemo::common::Logger::Log(::kvmemo::common::LogLevel::kError, __FILE__, __LINE__, (msg)); \
  } while (0)

#define KV_LOG_FATAL(msg) \
  do { \
    ::kvmemo::common::Logger::Log(::kvmemo::common::LogLevel::kFatal, __FILE__, __LINE__, (msg)); \
  } while (0)



/**
 * This source code may not be copied, modified, or
 * distributed without explicit permission from the author.
 */