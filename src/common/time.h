#pragma once
/**
 *  @file time.h
 *  @brief Time utilities for KVMemo
 * 
 *   This header provides a minimal, production-style abstraction for a time access.
 * 
 *  Design goals:
 *  - Centralize time access for consistency across modules.
 *  - Use a monotonic clock for durations and latency (std::chrono::steady_clock).
 *  - Provide a wall-clock timestamp (epoch milliseconds) for TTL expiration metadata.
 *  - Keep the API stable and minimal.
 *
 *  Notes:
 *  - TTL expiration requires a wall-clock timestamp (epoch time).
 *  - Latency measurement must use a monotonic clock to avoid issues when system
 *    time changes (NTP adjustments, manual changes, daylight savings).
 *
 *  Copyright © 2026 Gagan Bansal
 *  ALL RIGHT RESERVED
 */ 

#include <chrono>
#include <cstdint>

using namespace std;

namespace kvmemo::common {
    /**
     * @brief Strongly-typed alias for milliseconds since Unix epoch
     * 
     *  Used for : 
     *  - TTL expire_at timestamps
     *  - LRU last_access timestamps
     */

    using EpochMillis = uint64_t;

    /**
     * @brief Strongly-typed alias for monotonic time points.
     * 
     *  Used for : 
     *  - measuring latency
     *  - measuring durations between events
     */
    using SteadyTimePoints = chrono::steady_clock::time_point;

    /**
     * @brief Strongly-typed alias for monotonic durations in milliseconds 
     */
    using DurationMillis = chrono::milliseconds;

    /**
     * @class Clock
     * @brief Centralized time provider for KVMemo
     * 
     * This is intentionally a static-only utility class :
     *  - no instance
     *  - no state
     */
    class Clock final {
        public:
        Clock() = delete;

        [[nodiscard]] static EpochMillis NowEpochMillis() noexcept {
            const auto now = chrono::system_clock::now();
            const auto ms = chrono::duration_cast<chrono::milliseconds>(now.time_since_epoch());
            return static_cast<EpochMillis>(ms.count());
        }

        [[nodiscard]] static SteadyTimePoints NowSteady() noexcept {
            return chrono::steady_clock::now();
        }

        [[nodiscard]] static EpochMillis ElapsedMillis(SteadyTimePoints start, 
        SteadyTimePoints end) noexcept {
            const auto diff = chrono::duration_cast<chrono::milliseconds>(end - start);
            return static_cast<EpochMillis>(diff.count());
        }
    };       

} // namespace kvmemo::common

/**
 * This source code may not be copied, modified, or
 * distributed without explicit permission from the author.
 */
