#pragma once

/**
 * @file connection_manager.h
 * @brief Manages lifecycle and storage of active client connections.
 *
 * Responsibilities
 *  - Store active client connections
 *  - Add new connections accepted by the server
 *  - Remove closed connections
 *  - Provide lookup of connections by socket descriptor 
 *
 * Thread Safety  
 *  - Not thread-safe
 *  - Intended to be used within single event loop
 *
 *  Copyright © 2026 KVMemo
 *  Author: Gagan Bansal
 *  ALL RIGHTS RESERVED.
 */

#include <unordered_map>
#include <memory>
#include <stdexcept>

#include "connection.h"

namespace kvmemo::net {

/**
 * @brief Stores and manages active network connections.
 */
class ConnectionManager final {
public:

    ConnectionManager() = default;

    ConnectionManager(const ConnectionManager&) = delete;
    ConnectionManager& operator=(const ConnectionManager&) = delete;

    ConnectionManager(ConnectionManager&&) noexcept = default;
    ConnectionManager& operator=(ConnectionManager&&) noexcept = default;

    ~ConnectionManager() = default;

    /**
     * @brief Adds a new connection.
     */
    void Add(std::unique_ptr<Connection> connection)
    {
        int fd = connection->FD();

        connections_.emplace(fd, std::move(connection));
    }

    /**
     * @brief Removes a connection.
     */
    void Remove(int fd)
    {
        auto it = connections_.find(fd);

        if (it != connections_.end()) {
            connections_.erase(it);
        }
    }

    /**
     * @brief Returns connection by socket descriptor.
     *
     * Throws std::runtime_error if connection not found.
     */
    Connection* Get(int fd)
    {
        auto it = connections_.find(fd);

        if (it == connections_.end()) {
            throw std::runtime_error("Connection not found");
        }

        return it->second.get();
    }

    /**
     * @brief Returns number of active connections.
     */
    std::size_t Size() const noexcept
    {
        return connections_.size();
    }

    /**
     * @brief Iterates over all active connections, invoking callback(fd, connection*).
     */
    template <typename Callback>
    void ForEachConnection(Callback&& callback)
    {
        for (auto& [fd, conn] : connections_) {
            callback(fd, conn.get());
        }
    }

private:

    std::unordered_map<int, std::unique_ptr<Connection>> connections_;
};

} // namespace kvmemo::net

/**
 * This source code may not be copied, modified, or
 * distributed without explicit permission from the author.
 */