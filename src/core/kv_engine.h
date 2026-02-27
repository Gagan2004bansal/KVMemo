#pragma once
/**
 * @file kv_engine.h
 * @brief Top-Level key-value engine orchestration layer.
 * 
 * Responsibilities 
 *  - Exposes public KV operations (Set, Get, Delete).
 *  - Coordinates ShardManager.
 *  - Coordinates eviction policies.
 *  - Provides a clean boundary for server layer.
 * 
 *  Thread Safety
 *  > Thread-Safe
 *  > Delegates synchronization to shard layer.
 * 
 *  Copyright © 2026 Gagan Bansal
 *  ALL RIGHT RESERVED
 */

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "../common/time.h"
#include "shard_manager.h"
#include "ttl_index.h"
#include "../eviction/eviction_manager.h"

namespace kvmemo::core {
    class KVEngine {
        public: 
        /**
         * @brief Constructs KVEngine with required dependencies.
         */
        KVEngine(std::unique_ptr<ShardManager> shard_manager, 
                 std::unique_ptr<TTLIndex> ttl_index, 
                 std::unique_ptr<eviction::EvictionManager> eviction_manager)
                : shard_manager_(std::move(shard_manager)),
                ttl_index_(std::move(ttl_index)),
                eviction_manager_(std::move(eviction_manager)) {}

        KVEngine(const KVEngine&) = delete;
        KVEngine& operator=(const KVEngine&) = delete;
        KVEngine(KVEngine&&) noexcept = default;
        KVEngine& operator=(KVEngine&&) noexcept = default;
        ~KVEngine() = default;

        /**
         * @brief Stores a key-value pair
         * 
         *  @param key   Key String
         *  @param value Value String 
         *  @param ttl_ms Optional TTL in milliseconds
         */ 
        void Set(const std::string& key,
        const std::string& value, std::optional<uint64_t> ttl_ms = std::nullopt){

            if(ttl_ms.has_value()) {
                shard_manager_->SetWithTTL(key, value, ttl_ms.value());

                std::uint64_t expire_at = 
                    common::Clock::NowEpochMillis() + ttl_ms.value();

                ttl_index_->Upsert(key, expire_at);
            }
            else {
                shard_manager_->Set(key, std::move(value));
                ttl_index_->Remove(key);
            }

            eviction_manager_->OnWrite(key);
        }

        /**
         * @brief Retrives value for key.
         */
        std::optional<std::string> Get(const std::string& key) {

            auto value = shard_manager_->Get(key);
            if(value.has_value()) {
                eviction_manager_->OnRead(key);
            } 

        
            return value;
        }

        /**
         * @brief Deletes a key.
         */
        void Delete(const std::string& key) {
            shard_manager_->Delete(key);
            ttl_index_->Remove(key);
            eviction_manager_->OnDelete(key);
        }

        /**
         * @brief Expires keys that are due.
         * Called by TTL manager thread.
         */
        void ProcessExpired() {
            std::uint64_t now = common::Clock::NowEpochMillis();
            auto expired_keys = ttl_index_->CollectExpired(now);

            for(const auto& key : expired_keys) {
                shard_manager_->Delete(key);
                eviction_manager_->OnDelete(key);
            }
        }

        void ProcessEvictions() {
            auto victims = eviction_manager_->CollectEvictionCandidates();

            for(const auto& key : victims) {
                shard_manager_->Delete(key);
                ttl_index_->Remove(key);
            }
        }

    private:
        std::unique_ptr<ShardManager> shard_manager_;
        std::unique_ptr<TTLIndex> ttl_index_;
        std::unique_ptr<eviction::EvictionManager> eviction_manager_;
    };
} // namespace kvmemo::core

/**
 * This source code may not be copied, modified, or
 * distributed without explicit permission from the author.
 */