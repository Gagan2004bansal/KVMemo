#pragma once 
 /**
  * @file lru_cache.h
  * @brief Implements a non-thread safe LRU eviction structure.
  * 
  * Responsibilities :
  *    - Track key usage order.
  *    - Provide O(1) access updates.
  *    - Provide O(1) eviction candidate retrieval
  * 
  *  Design Principles : 
  *   > SRP : only manages recency ordering.
  *   > No internal synchronoization (handled by shard).
  *   > O(1) operations using hashmap + doubly linked list.
  *   > No dynamic polymorphism.
  * 
  *   Thread Safety :
  *   => Not thread-safe
  *   => Caller must ensure synchronization.
  *  
  *  Copyright © 2026 Gagan Bansal
  *  ALL RIGHT RESERVED
  */

#include <list>
#include <unordered_map>
#include <string>
#include <cstddef>
#include <stdexcept>

namespace kvmemo::core {
    /**
     * @brief LRU cache index for key tracking.
     * 
     *  This class does NOT store values.
     *  It only tracks keys in recency order.
     *  
     *  Most recently used key  -> Front
     *  Least recently used key -> Back
     */
    class LRUCache final {
        public: 
        using Key = std::string;

        explicit LRUCache(size_t  capacity) : capacity_(capacity)
        {
            if(capacity_ == 0){
                throw std::invalid_argument("LRU capacity must be greater than zero");
            }
        }

        LRUCache(const LRUCache&) = delete;
        LRUCache& operator=(const LRUCache&) = delete;

        LRUCache(LRUCache&&) noexcept = default;
        LRUCache& operator=(LRUCache&&) noexcept = default;
        
        ~LRUCache() = default;

        /**
         * @brief Marks key as recently used.
         * 
         *  If key exists -> move to front.
         *  If Key does not exist -> Insert at front.
         * 
         *  @return true if insertion caused overflow (eviction needed).
         */
        bool Touch(const Key& key){
            auto it = map_.find(key);

            if(it != map_.end()){
                order_.splice(order_.begin(), order_, it->second);
                return false;
            }

            order_.push_front(key);
            map_[key] = order_.begin();

            return map_.size() > capacity_;
        }

        /**
         * @brief Removes a key from tracking.
         */
        void Remove(const Key& key){
            auto it = map_.find(key);
            if(it == map_.end()) {
                return;
            }

            order_.erase(it->second);
            map_.erase(it);
        }

        /**
         * @brief Returns the least recently used key.
         * Caller must ensure cache is not empty.
         */
        const Key& EvictionCandidate() const {
            if(order_.empty()) {
                throw std::runtime_error("LRUCache is empty");
            }
            return order_.back();
        }

        /**
         * @brief Removes and returns LRU key.
         */
        Key PopEvictionCandidate() {
            if (order_.empty()) {
                throw std::runtime_error("LRUCache is empty");
            }

            Key lru_key = order_.back();
            order_.pop_back();
            map_.erase(lru_key);
            return lru_key;
        }

        /**
         * Returns current tracked size.
         */
        size_t Size() const noexcept {
            return map_.size();
        }

        /**
         * @brief Returns configured capacity.
         */
        size_t Capacity() const noexcept {
            return capacity_;
        }

        /**
         * @brief Clears all tracking state.
         */
        void Clear() noexcept {
            order_.clear();
            map_.clear();
        }

        private:
        size_t capacity_;

        std::list<Key> order_;
        std::unordered_map<Key, typename std::list<Key>::iterator> map_;
    };
} // namespace kvmemo::core

 /**
 * This source code may not be copied, modified, or
 * distributed without explicit permission from the author.
 */