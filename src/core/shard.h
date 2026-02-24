#pragma once
/**
 * @file shard.h
 * @brief Respresents a single shard in the KV engine.
 *
 *  Responsibilities :
 *  - Store Key -> Entry mappings
 *  - Enforce thread-safety at shard level
 *  - Integrate LRU eviction tracking
 *  - Provide atomic key operations
 *
 *  Thread Safety :
 *  > Fully thread-safe using internal mutex.
 *  > All public APIs are safe for concurrent access.
 *
 *  Copyright © 2026 Gagan Bansal
 *  ALL RIGHT RESERVED
 */

#include <unordered_map>
#include <mutex>
#include <optional>
#include <string>
#include <vector>
#include <cstddef>

#include "entry.h"
#include "lru_cache.h"
#include "ttl_index.h"

namespace kvmemo::core
{

    class Shard final
    {
        public:
        using Key = std::string;

        explicit Shard(std::size_t capacity)
            : capacity_(capacity),
              lru_(capacity),
              ttl_index_() {}

        Shard(const Shard &) = delete;
        Shard &operator=(const Shard &) = delete;

        Shard(Shard &&) = delete;
        Shard &operator=(Shard &&) = delete;

        ~Shard() = default;

        /**
         * @brief Insert or Update key without TTL.
         */
        void Set(const Key &key, std::string value)
        {
            std::lock_guard<std::mutex> lock(mutex_);

            store_[key] = Entry(std::move(value));

            bool overflow = lru_.Touch(key);
            ttl_index_.Remove(key);

            if (overflow)
            {
                EvictOne();
            }
        }

        /**
         * @brief Insert or update key with TTL (milliseconds).
         */
        void SetWithTTL(const Key &key, std::string value, std::uint64_t ttl_ms)
        {
            std::lock_guard<std::mutex> lock(mutex_);

            Entry entry(std::move(value), ttl_ms);
            store_[key] = entry;

            bool overflow = lru_.Touch(key);

            if (entry.HasTTL())
            {
                ttl_index_.Upsert(key, entry.ExpireAt());
            }

            if (overflow)
            {
                EvictOne();
            }
        }

        /**
         * @brief Retrive value by key
         *
         * Return nullopt if
         *  - Key not found
         *  - Key expired
         */
        std::optional<std::string> Get(const Key &key)
        {
            std::lock_guard<std::mutex> lock(mutex_);

            auto it = store_.find(key);

            if (it == store_.end())
            {
                return std::nullopt;
            }

            if (it->second.IsExpired())
            {
                RemoveInternal(key);
                return std::nullopt;
            }

            lru_.Touch(key);
            return it->second.Value();
        }

        /**
         * @brief Remove Key from shard.
         */
        void Delete(const Key &key)
        {
            std::lock_guard<std::mutex> lock(mutex_);
            RemoveInternal(key);
        }

        /**
         * @brief Returns number of stored keys.
         */
        std::size_t Size() const
        {
            std::lock_guard<std::mutex> lock(mutex_);
            return store_.size();
        }

        /**
         * @brief Performs TTL cleanup for expired keys.
         */
        void CleanupExpired(std::uint64_t now)
        {
            std::lock_guard<std::mutex> lock(mutex_);

            auto expired_keys = ttl_index_.CollectExpired(now);

            for (const auto &key : expired_keys)
            {
                RemoveInternal(key);
            }
        }

        void RemoveInternal(const Key &key)
        {
            store_.erase(key);
            lru_.Remove(key);
            ttl_index_.Remove(key);
        }

        void EvictOne()
        {
            if (store_.empty())
            {
                return;
            }

            Key victim = lru_.PopEvictionCandidate();
            store_.erase(victim);
            ttl_index_.Remove(victim);
        }

        private:
            const std::size_t capacity_;
            mutable std::mutex mutex_;

            std::unordered_map<Key, Entry> store_;
            LRUCache lru_;
            TTLIndex ttl_index_;
    };
} // namespace kvmemo::core

/**
 * This source code may not be copied, modified, or
 * distributed without explicit permission from the author.
 */