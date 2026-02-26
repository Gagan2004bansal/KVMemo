#pragma once 
/**
 * @file memory_tracker.h
 * @brief Track approximate memory usage of the KV Engine.
 * 
 * Responsibilities : 
 *  - Track current memory consumption.
 *  - Enforce memory limits.
 *  - Provide atomic updates for concurrent environment.
 * 
 * Thread Safety :
 *  - Fully Thread safe via atomic counters. 
 * 
 *  Copyright © 2026 Gagan Bansal
 *  ALL RIGHT RESERVED
 */

#include <atomic>
#include <cstddef>
#include <stdexcept>

namespace kvmemo::eviction {
    /**
     * @brief Track approximate memory usage of the system.
     * 
     * This tracker does not peform deep objects introspection.
     * It relies on upper layers(Shard, Entry) to report memory deltas.
     * 
     *  Memory Accounting Model : 
     *  - Value Size
     *  - Key Size
     *  - Metadata overhead
     */
    class MemoryTracker final {
        public: 
        /**
         * @brief Construct MemoryTracker
         * @param max_memory_bytes Maximum allowed memory
         */
        explicit MemoryTracker(std::size_t max_memory_bytes)
            : max_memory_bytes_(max_memory_bytes),
            current_memory_bytes_(0)
        {
            if(max_memory_bytes_ == 0) {
                throw std::invalid_argument("Max memory must be greater than zero");
            }
        }

        MemoryTracker(const MemoryTracker&) = delete;
        MemoryTracker& operator=(const MemoryTracker&) = delete;

        MemoryTracker(MemoryTracker&&) = delete;
        MemoryTracker& operator=(MemoryTracker&&) = delete;

        ~MemoryTracker() = default;

        /**
         * @brief Attempts to reserve memory.
         * 
         * @param bytes Number of bytes to add.
         * @return true if within limit after reservation.
         */
        bool Reserve(std::size_t bytes) noexcept {
            current_memory_bytes_.fetch_add(bytes, std::memory_order_relaxed);
            return !IsOverLimit();
        }

        /**
         * @brief Release memory
         * @param bytes Number of bytes to subtract.
         */
        void Release(std::size_t bytes) noexcept {
            current_memory_bytes_.fetch_sub(bytes, std::memory_order_relaxed);
        }

        /**
         * @brief Returns current memory usage.
         */
        std::size_t CurrentUsage() const noexcept {
            return current_memory_bytes_.load(std::memory_order_relaxed);
        }

        /**
         * @brief Returns configured memory limit.
         */
        std::size_t MaxLimit() const noexcept {
            return max_memory_bytes_;
        }

        /**
         * @brief Returns true if memory exceeds configured limits.
         */
        bool IsOverLimit() const noexcept {
            return CurrentUsage() > max_memory_bytes_;
        }

    private:
        const std::size_t max_memory_bytes_;
        std::atomic<std::size_t> current_memory_bytes_;
    };
} // namespace kvmemo::eviction

/**
 * This source code may not be copied, modified, or
 * distributed without explicit permission from the author.
 */