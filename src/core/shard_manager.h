#pragma once

/**
 *  @file shard_manager.h
 *  @brief Manages multiple shards inside the KV engine.
 * 
 *  Responsibilities : 
 *  - Distribute keys across shards
 *  - Provide shard-level routing
 *  - Enable parallelism and scalability
 *  - Maintain consistent hashing strategy
 *  
 *  Thread Safety : 
 *  - Thread safe by delegation.
 *  - Individual shard operations are internally synchronized.
 * 
 *  Copyright © 2026 Gagan Bansal
 *  ALL RIGHT RESERVED
 */

#include <vector>
#include <memory>
#include <string>
#include <functional>
#include <stdexcept>

#include "shard.h"

namespace kvmemo::core {
    class ShardManager final {
        public:
            using Key = std::string;

            /**
             * @brief ShardManager
             * 
             * @param shard_count Number of shards (must be > 0)
             * @param shard_capacity Capacity per shard
             */
            ShardManager(std::size_t shard_count, std::size_t shard_capacity) : shard_count_(shard_count) {
                if(shard_count == 0) {
                    throw std::invalid_argument("Shard count must be greater than zero");
                }

                shards_.reserve(shard_count_);
                for(std::size_t i = 0; i<shard_count_; ++i) {
                    shards_.emplace_back(std::make_unique<Shard>(shard_capacity));
                }
            }

            ShardManager(const ShardManager&) = delete;
            ShardManager& operator=(const ShardManager&) = delete;

            ShardManager(ShardManager&&) = delete;
            ShardManager& operator=(ShardManager&&) = delete;

            ~ShardManager() = default;

        /**
         * @brief Insert or update key without TTL.
         */
        void Set(const Key& key, std::string value) {
            GetShard(key).Set(key, std::move(value));
        }

        /**
         * @brief Insert or update key with TTL (milliseconds).
         */
        void SetWithTTL(const Key& key, std::string value, std::uint64_t ttl_ms) {
            GetShard(key).SetWithTTL(key, std::move(value), ttl_ms);
        }

        /**
         * @brief Retrive value by key.
         */
        std::optional<std::string> Get(const Key& key) {
            return GetShard(key).Get(key);
        }

        /**
         * @brief Delete key.
         */
        void Delete(const Key& key) {
            GetShard(key).Delete(key);
        }

        /**
         * @brief Run TTL cleanup across all shards.
         */
        void CleanupExpired(std::uint64_t now) {
            for (auto& shard : shards_) {
                shard->CleanupExpired(now);
            }
        }

        /**
         * @brief Total number of shards.
         */
        std::size_t ShardCount() const noexcept {
            return shard_count_;
        }

    private:
        /**
         * @brief Determines shard index for a given key.
         */
        Shard& GetShard(const Key& key) {
            std::size_t index = hasher_(key) % shard_count_;
            return *shards_[index];
        }

        const std::size_t shard_count_;
        std::vector<std::unique_ptr<Shard>> shards_;
        std::hash<Key> hasher_;
    };
} // namespace kvmemo::core

/**
 * This source code may not be copied, modified, or
 * distributed without explicit permission from the author.
 */