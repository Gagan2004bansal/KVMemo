#pragma once
/**
 * @file tcp_server.h
 * @brief TCP server responsible for accepting client connections.
 *
 *  Responsibilities :
 * - Create and configure listening socket.
 * - Accept incoming client connections.
 * - Create Connection objects.
 * - Register connections with ConnectionManager.
 *
 *  Thread Safety :
 *  > Not Thread-Safe.
 *  > Intended to run inside single server thread.
 *
 *  Copyright © 2026 KVMemo
 *  Author: Gagan Bansal
 *  ALL RIGHTS RESERVED.
 */

#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cstring>
#include <stdexcept>

#include "connection.h"
#include "connection_manager.h"

namespace kvmemo::net
{
    /**
     * @brief Basic TCP server implementation.
     */
    class TcpServer final
    {
    public:
        explicit TcpServer(int port) : port_(port) {};

        TcpServer(const TcpServer &) = delete;
        TcpServer &operator=(const TcpServer &) = delete;

        TcpServer(TcpServer &&) noexcept = default;
        TcpServer &operator=(TcpServer &&) noexcept = default;

        ~TcpServer()
        {
            Stop();
        }

        /**
         * @brief Starts the TCP Server.
         */
        void Start()
        {
            CreateSocket();
            Bind();
            Listen();
        }

        /**
         * @brief Accepts a new client connection.
         */
        void Accept()
        {
            sockaddr_in client_addr{};
            socklen_t addr_len = sizeof(client_addr);

            int client_fd = ::accept(
                listen_fd_,
                reinterpret_cast<sockaddr *>(&client_addr),
                &addr_len);

            if (client_fd < 0)
            {
                throw std::runtime_error("Failed to accept connection");
            }

            auto conn = std::make_unique<kvmemo::net::Connection>(client_fd);
            connection_.Add(std::move(conn));
        }

        /**
         * @brief Stops the server and closes socket.
         */
        void Stop()
        {
            if (listen_fd_ >= 0)
            {
                ::close(listen_fd_);
                listen_fd_ = -1;
            }
        }

        /**
         * @brief Returns the listening socket file descriptor.
         */
        int ListenFD() const noexcept
        {
            return listen_fd_;
        }

        /**
         * @brief Returns connection manager.
         */
        ConnectionManager &Connection() noexcept
        {
            return connection_;
        }

    private:
        void CreateSocket()
        {
            listen_fd_ = ::socket(AF_INET, SOCK_STREAM, 0);

            if (listen_fd_ < 0)
            {
                throw std::runtime_error("Failed to create socket");
            }
        }

        void Bind()
        {
            sockaddr_in addr{};
            std::memset(&addr, 0, sizeof(addr));

            addr.sin_family = AF_INET;
            addr.sin_addr.s_addr = INADDR_ANY;
            addr.sin_port = htons(port_);

            if (::bind(listen_fd_, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) < 0)
            {
                throw std::runtime_error("Bind Failed");
            }
        }

        void Listen()
        {
            if (::listen(listen_fd_, 128) < 0)
            {
                throw std::runtime_error("Listen Failed");
            }
        }

    private:
        int port_;
        int listen_fd_{-1};

        ConnectionManager connection_;
    };
} // namespace kvmemo::net

/**
 * This source code may not be copied, modified, or
 * distributed without explicit permission from the author.
 */