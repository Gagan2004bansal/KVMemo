#pragma once
/**
 * @file config.h
 * @brief Global configuration for KVMemo.
 *
 * This header defines the configuration struct used across KVMemo.
 *
 * Design goals:
 *  - Keep configuration centralized and strongly typed.
 *  - Avoid global mutable state.
 *  - Ensure values are validated early.
 *  - Keep defaults safe for development and scalable for production.
 *
 *  Copyright © 2026 Gagan Bansal
 *  ALL RIGHT RESERVED
 */

#include <cstddef>
#include <cstdint>

#include "src/common/status.h"

namespace kvmemo::common {

/**
 * @enum EvictionPolicy
 * @brief Defines how KVMemo evicts keys when capacity is exceeded.
 *
 * Notes:
 *  - We support LRU as the primary policy.
 *  - TTL is not an eviction policy; it is expiration logic.
 */
enum class EvictionPolicy : std::uint8_t {
  kNone = 0,
  kLRU = 1,
};

/**
 * @struct Config
 * @brief Central configuration object for KVMemo.
 *
 * This is intentionally a simple POD-like struct with:
 *  - explicit defaults
 *  - a Validate() method
 *
 * It must remain stable because it will be referenced across many modules.
 */
struct Config final {
  /**
   * @brief Number of independent shards for the in-memory store.
   */
  std::size_t shard_count = 64;

  /**
   * @brief Maximum memory allowed for the in-memory store (bytes).
   *
   * This is the global limit across all shards.
   * When exceeded, eviction is triggered based on the configured policy.
   *
   * Default: 256 MB (safe for laptops and dev machines).
   */
  std::uint64_t max_memory_bytes = 256ULL * 1024ULL * 1024ULL;

  /**
   * @brief Maximum size of a single value stored in the KV store (bytes).
   *
   * This prevents pathological memory usage and protects against malicious
   * clients sending extremely large payloads.
   *
   * Default: 8 MB.
   */
  std::uint64_t max_value_bytes = 8ULL * 1024ULL * 1024ULL;

  /**
   * @brief TCP server listen port.
   *
   * KVMemo will expose a custom TCP protocol.
   */
  std::uint16_t listen_port = 8080;

  /**
   * @brief Maximum number of simultaneous client connections.
   *
   * This is a soft limit enforced by the server layer.
   * Exceeding this will result in connection rejection.
   */
  std::size_t max_connections = 4096;

  /**
   * @brief Number of worker threads for handling client requests.
   */
  std::size_t worker_threads = 0;

  /**
   * @brief Enables TTL support.
   *
   * If disabled:
   *  - keys never expire automatically
   *  - TTL commands can return errors
   */
  bool enable_ttl = true;

  /**
   * @brief Interval (in milliseconds) for the TTL expiry sweep thread.
   * Default: 250ms (responsive enough without burning CPU).
   */
  std::uint32_t ttl_sweep_interval_ms = 250;

  /**
   * @brief Enables metrics collection.
   *
   * If enabled, KVMemo tracks:
   *  - QPS
   *  - latency percentiles (later)
   *  - memory usage
   *  - eviction counts
   *  - TTL expiry counts
   *
   * Metrics should be low-overhead and thread-safe.
   */
  bool enable_metrics = true;

  /**
   * @brief Configures the eviction policy.
   *
   * Default: LRU.
   */
  EvictionPolicy eviction_policy = EvictionPolicy::kLRU;

  /**
   * @brief Validates the configuration.
   *
   * This must be called once during startup before constructing components.
   *
   * @return Status::Ok() if valid, otherwise a descriptive error status.
   */
  [[nodiscard]] Status Validate() const noexcept {
    if (shard_count == 0) {
      return Status::InvalidArgument("Config.shard_count must be > 0");
    }

    // For performance and simplicity, we require shard_count to be a power of two.
    // This allows fast shard selection using bit-masking.
    if ((shard_count & (shard_count - 1)) != 0) {
      return Status::InvalidArgument(
          "Config.shard_count must be a power of two (e.g., 16, 32, 64)");
    }

    if (max_memory_bytes == 0) {
      return Status::InvalidArgument("Config.max_memory_bytes must be > 0");
    }

    if (max_value_bytes == 0) {
      return Status::InvalidArgument("Config.max_value_bytes must be > 0");
    }

    if (max_value_bytes > max_memory_bytes) {
      return Status::InvalidArgument(
          "Config.max_value_bytes must be <= Config.max_memory_bytes");
    }

    if (listen_port == 0) {
      return Status::InvalidArgument("Config.listen_port must be a valid port");
    }

    if (max_connections == 0) {
      return Status::InvalidArgument("Config.max_connections must be > 0");
    }

    // If worker_threads == 0, we treat it as auto-detect later.
    // But if explicitly set, it must be reasonable.
    if (worker_threads > 0 && worker_threads > 1024) {
      return Status::InvalidArgument(
          "Config.worker_threads is too high; must be <= 1024");
    }

    if (enable_ttl) {
      if (ttl_sweep_interval_ms == 0) {
        return Status::InvalidArgument(
            "Config.ttl_sweep_interval_ms must be > 0 when TTL is enabled");
      }
    }

    // Eviction policy validation.
    switch (eviction_policy) {
      case EvictionPolicy::kNone:
      case EvictionPolicy::kLRU:
        break;
      default:
        return Status::InvalidArgument("Config.eviction_policy is invalid");
    }

    return Status::Ok();
  }
};

}  // namespace kvmemo::common

/**
 * This source code may not be copied, modified, or
 * distributed without explicit permission from the author.
 */
