#pragma once 

/**
 * @file eviction_manager.h
 * @brief Central eviction policy coordinator.
 * 
 *  Responsibilities :
 *  - Tracks read/write/delete access patterns
 *  - Consults memory limits
 *  - Selects keys for eviction (via policy)
 *  - Notifies KVEngine when eviction is required
 * 
 *  Thread Safety :
 *  > Thread-Safe
 *  > Internal synchronization for policy tracking
 * 
 *  Copyright © 2026 Gagan Bansal
 *  ALL RIGHT RESERVED
 */

#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

#include "../common/status.h"
#include "../core/lru_cache.h"
#include "memory_tracker.h"

namespace kvmemo::eviction {
/**
 * @brief Abstract eviction policy interface.
 */
class EvictionPolicy {
    public:
        virtual ~EvictionPolicy() = default;

        virtual void OnRead(const std::string& key) = 0;
        virtual void OnWrite(const std::string& key) = 0;
        virtual void OnDelete(const std::string& key) = 0;

    /**
     *  @brief Selects candidate key for eviction.
     */
    virtual std::optional<std::string> SelectVictim() = 0;
};

/**
 * @brief Default LRU eviction policy implementation
 */
class LRUPolicy final : public EvictionPolicy {
    public:
    explicit LRUPolicy(std::unique_ptr<core::LRUCache> lru) : lru_(std::move(lru)) {}

    void OnRead(const std::string& key) override {
        lru_->Touch(key);
    } 

    void OnWrite(const std::string& key) override {
        lru_->Touch(key);
    }

    void OnDelete(const std::string& key) override {
        lru_->Remove(key);
    }

    std::optional<std::string> SelectVictim() override {
        return lru_->PopEvictionCandidate();
    }

private:
    std::unique_ptr<core::LRUCache> lru_;
};

/**
 * @brief Eviction manager coordinating memory and policy.
 */
class EvictionManager {
    public: 
    EvictionManager(std::unique_ptr<MemoryTracker> memory_tracker, 
                 std::unique_ptr<EvictionPolicy> policy)
                 : memory_tracker_(std::move(memory_tracker)),
                 policy_(std::move(policy)) {}

    EvictionManager(const EvictionManager&) = delete;
    EvictionManager& operator=(const EvictionManager&) = delete;
    ~EvictionManager() = default;

    /**
     * @brief Called when a key is read.
     */
    void OnRead(const std::string& key) {
        std::lock_guard<std::mutex> lock(mutex_);
        policy_->OnRead(key);
    }

    /**
     * @brief Called when a key is written.
     * May trigger eviction if memory exceeded.
     */
    void OnWrite(const std::string& key) {
        std::lock_guard<std::mutex> lock(mutex_);

        memory_tracker_->OnAllocation();
        policy_->OnWrite(key);

        EnforceMemoryLimit();
    }

    /**
     * @brief Called when a key is deleted.
     */
    void OnDelete(const std::string& key) {
        std::lock_guard<std::mutex> lock(mutex_);

        memory_tracker_->OnDeallocation();
        policy_->OnDelete(key);
    } 

    /**
     * @brief Returns keys that must be evicted.
     */
    std::vector<std::string> CollectEvictionCandidates() {
        std::lock_guard<std::mutex> lock(mutex_);

        std::vector<std::string> victims;

        while(memory_tracker_->IsLimitExceeded()) {
            auto candidate = policy_->SelectVictim();

            if(!candidate.has_value()) {
                break;
            }

            victims.push_back(candidate.value());
            memory_tracker_->OnDeallocation();
        }

        return victims;
    }

    private:
        void EnforceMemoryLimit() {
            if(!memory_tracker_->IsLimitExceeded()) {
                return;
            }
        }

    std::unique_ptr<MemoryTracker> memory_tracker_;
    std::unique_ptr<EvictionPolicy> policy_;
    std::mutex mutex_;
};
} // namespace kvmemo::eviction
/**
 * This source code may not be copied, modified, or
 * distributed without explicit permission from the author.
 */