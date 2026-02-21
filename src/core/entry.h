#pragma once
 
/**
 *  @file Entry.h
 *  @brief Represents a single key-value record inside a shard
 * 
 *  This class encapsulates :
 *  - Value storage (binary safe)
 *  - Expiration timestamp (TTL support)
 *  - Creation Timestamp
 *  - LightWeight metadata hooks
 * 
 *  Thread Safety :
 *      Entry itself is NOT internally synchronized =.
 *      Synchronized is handled at shard level.
 * 
 *  Copyright © 2026 Gagan Bansal
 *  ALL RIGHT RESERVED
 */

#include <string>
#include <cstdint>
#include <atomic>
#include "../common/time.h"

using namespace std;

namespace kvmemo::core {

    /**
     * @brief Represent a stored value inside thr KV engine.
     * 
     *  Entry is intenionally lightweight. It does not store the key.
     *  The key is owned by the shard's unordered_map.
     * 
     *  Memory layout Considerations : 
     *  > Keep frequently accessed fields close.
     *  > Avoid heap fragmentation beyond std::string.   
     */
    class Entry final {
        public: 
        using Timestamp = std::uint64_t;

        /**
         * @brief Contruct a non-expiring entry.
         */
        explicit Entry(string value) : value_(move(value)),
                                        created_at_(common::Time::NowMillis()),
                                        expire_at_(0) {}

        Entry(string value, std::uint64_t ttl_ms) : value_(move(value)),
                                        created_at_(common::Time::NowMillis()),
                                        expire_at(ttl_ms == 0 ? 0 : created_at_ + ttl_ms) {}
    

        Entry(const Entry&) = default;
        Entry(Entry&&) noexcept = default;
        Entry& operator=(const Entry&) = default;
        Entry& operator=(Entry&&)noexcept = default;
        ~Entry() = default;

        /**
         * @brief Returns stored value.
         */
        const string& Value() const noexcept {
            return value_;
        }

        /**
         * @brief Updates value and optionally TTL.
         */
        void Update(string new_value, uint64_t ttl_ms = 0) {
            value_ = move(new_value);
            created_at_ = common::Time::NowMillis();
            expire_at_ = ttl_ms == 0 ? 0 : created_at_ + ttl_ms;
        }

        /**
         * @brief Returns true if entry has expiration configured.
         */
        bool HasTTL() const noexcept {
            return expire_at_ != 0;
        }

        /**
         * @brief Returns expiration timestamp (0 if no TTL)
         */
        Timestamp ExpireAt() const noexcept {
            return expire_at_;
        }

        /**
         * @brief Returns creation timestamp.
         */
        Timestamp CreatedAt() const noexcept {
            return created_at_;
        }

        /**
         * @brief Returns true if entry is expired.
         */
        bool isExpired() const noexcept {
            if(expire_at_ == 0) {
                return false;
            }
            return common::Time::NowMillis() >= expire_at_;
        }

        /**
         * @brief Returns remaining TTL in milliseconds.
         * - 0 if no TTl
         * - 0 if already expired
         * - Remaining milliseconds otherwise
         */

        uint64_t RemainingTTL() const noexcept {
            if(expire_at_ == 0) {
                return 0;
            }

            const auto now = common::Time::NowMillis();
            if(now >= expire_at_) {
                return 0;
            }

            return expire_at_ - now;
        }

        private:
        string value_;  
        Timestamp created_at_;
        Timestamp expire_at_;
    }
}

/**
 * This source code may not be copied, modified, or
 * distributed without explicit permission from the author.
 */
