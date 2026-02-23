#pragma once 

/**
 * @file ttl_index.h
 * @brief Maintains time-ordered expiration tracking for keys with TTL.
 * 
 *  Responsibilities :
 *  - Track expiration timestamps .
 *  - Provide efficient retrival of expired keys.
 *  
 *   Thread Safety :
 *  > NOT thread-safe.
 *  > Caller must ensure synchronization.
 * 
 *  Copyright © 2026 Gagan Bansal
 *  ALL RIGHT RESERVED
 */

#include <map>
#include <unordered_map>
#include <string>
#include <vector>
#include <cstdint>


namespace kvmemo::core {

    /**
     * @brief TLL index for expiration management.
     * 
     *  Maintains : expire_at => set of keys.
     * 
     *  Shard is responsibe for actual deletion.
     */
    class TTLIndex final {
        public:
        using Key = std::string;
        using Timestamp = std::uint64_t;

        TTLIndex() = default;

        TTLIndex(const TTLIndex&) = delete;
        TTLIndex& operator=(const TTLIndex&) = delete;

        TTLIndex(TTLIndex&&) noexcept = default;
        TTLIndex& operator=(TTLIndex&&) noexcept = default;

        ~TTLIndex() = default;

        /**
         * @brief Add or update TTL for a key.
         * If key already exists, previous timestamp is removed.
         */
        void Upsert(const Key& key, Timestamp expire_at) {
            Remove(key);

            expiry_map_[expire_at].push_back(key);
            key_index_[key] = expire_at;
        }

        /**
         * @brief Remove key from TTL tracking.
         */
        void Remove(const Key& key) {
            auto it = key_index_.find(key);
            if(it == key_index_.end()) {
                return;
            }

            Timestamp ts = it->second;
            auto map_it = expiry_map_.find(ts);

            if(map_it != expiry_map_.end()) {
                auto& keys = map_it->second;
            
                for(auto vec_it = keys.begin(); vec_it != keys.end(); ++vec_it) {
                    if(*vec_it == key) {
                        keys.erase(vec_it);
                        break;
                    }
                }

                if(keys.empty()) {
                    expiry_map_.erase(map_it);
                }
            }

            key_index_.erase(it);
        }

        /**
         * @brief Collect all expired keys up to given timestamps.
         */
        std::vector<Key> CollectExpired(Timestamp now) {
            std::vector<Key> expired_keys;

            auto it = expiry_map_.begin();
            while(it != expiry_map_.end() && it->first <= now) {
                for(const auto& key : it->second) {
                    expired_keys.push_back(key);
                    key_index_.erase(key);
                }

                it = expiry_map_.erase(it);
            }

            return expired_keys;
        }

        /**
         * @brief Returns number of tracked TTL keys.
         */
        std::size_t Size() const noexcept {
            return key_index_.size();
        }

        /**
         * @brief Clears entire TTL index.
         */
        void Clear() noexcept {
            expiry_map_.clear();
            key_index_.clear();
        }

        private:
        // expire_at -> left of keys
        std::map<Timestamp, std::vector<Key>> expiry_map_;

        // key -> expire_at
        std::unordered_map<Key, Timestamp> key_index_;
    };
} // namespace kvmemo::core


/**
 * This source code may not be copied, modified, or
 * distributed without explicit permission from the author.
 */