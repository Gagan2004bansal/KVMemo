#pragma once
/**
 *  Copyright © 2026 KVMemo
 *  Author: Gagan Bansal
 *  ALL RIGHTS RESERVED.
 */
#include <string>
#include <stdexcept>
#include <vector>
#include <sys/select.h>

#include "../net/tcp_server.h"
#include "../protocol/framing.h"
#include "../protocol/parser.h"
#include "../protocol/serializer.h"
#include "../protocol/response.h"
#include "../core/kv_engine.h"
#include "dispatcher.h"

namespace kvmemo::server
{
    /**
     * @brief Main server application
     */

    class ServerApp final
    {
    public:
        explicit ServerApp(int port) : server_(port), engine_(std::make_unique<core::ShardManager>(64, 10000),
                                                              std::make_unique<core::TTLIndex>(),
                                                              std::make_unique<eviction::EvictionManager>(
                                                                  std::make_unique<eviction::MemoryTracker>(256 * 1024 * 1024),
                                                                  std::make_unique<eviction::LRUPolicy>(
                                                                      std::make_unique<core::LRUCache>(10000)))),
                                       dispatcher_(engine_) {}

        ServerApp(const ServerApp &) = delete;
        ServerApp &operator=(const ServerApp &) = delete;

        ServerApp(ServerApp &&) noexcept = default;
        ServerApp &operator=(ServerApp &&) noexcept = delete;

        ~ServerApp() = default;

        /**
         * @brief Starts the server runtime loop.
         */
        void Run()
        {
            server_.Start();

            while (true)
            {
                ProcessConnections();
            }
        }

    private:
        /**
         * @brief Accepts new connections and processes requests for all active
         *        connections using select() for event-driven I/O.
         *
         *        The listening socket is included in the select() watch set so
         *        that the loop never blocks waiting for a new connection while
         *        existing clients are waiting for their commands to be handled.
         */
        void ProcessConnections()
        {
            auto &manager = server_.Connection();
            fd_set readfds;
            FD_ZERO(&readfds);

            int listen_fd = server_.ListenFD();
            FD_SET(listen_fd, &readfds);
            int max_fd = listen_fd;

            active_fds_.clear();
            manager.ForEachConnection([&](int fd, net::Connection *conn)
                                      {
                FD_SET(fd, &readfds);
                max_fd = std::max(max_fd, fd);
                active_fds_.push_back(fd); });

            struct timeval tv = {0, kSelectTimeoutUs};
            int activity = select(max_fd + 1, &readfds, nullptr, nullptr, &tv);

            if (activity < 0)
                return;

            if (activity > 0)
            {
                if (FD_ISSET(listen_fd, &readfds))
                {
                    server_.Accept();
                }

                for (int fd : active_fds_)
                {
                    if (FD_ISSET(fd, &readfds))
                    {
                        ConnectionSafeProcess(manager, fd);
                    }
                }
            }
        }

        void ConnectionSafeProcess(net::ConnectionManager &manager, int fd)
        {
            try
            {
                auto *conn = manager.Get(fd);

                if (!conn)
                {
                    return;
                }

                if (conn->ReadFromSocket() <= 0)
                {
                    manager.Remove(fd);
                    return;
                }

                std::string frame;

                while (protocol::Framing::NextFrame(conn->InputBuffer(), frame))
                {
                    auto request = protocol::Parser::Parse(frame);

                    protocol::Response response = dispatcher_.Dispatch(request);

                    std::string wire = protocol::Serializer::Serialize(response);

                    conn->OutputBuffer().Append(wire);

                    conn->WriteToSocket();
                }
            }
            catch (...)
            {
                manager.Remove(fd);
            }
        }

    private:
        static constexpr int kSelectTimeoutUs = 50000;

        Dispatcher dispatcher_;
        net::TcpServer server_;
        core::KVEngine engine_;

        std::vector<int> active_fds_;
    };
} // namespace kvmemo::server

/**
 * This source code may not be copied, modified, or
 * distributed without explicit permission from the author.
 */