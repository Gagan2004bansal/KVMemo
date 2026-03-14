#pragma once 
/**
 * @file request_context.h
 * @brief Represents the execution context for a single client request.
 * 
 * Responsibilities :
 * - Store request information during execution.
 * - Provide access to KVEngine.
 * - Provide access to network connection.
 * - Provide extensibility for metrics and tracing.
 * 
 * Thread Safety :
 * > Not thread safe.
 * > Each context instance is owned by a single request execution.
 *
 *  Copyright © 2026 KVMemo
 *  Author: Gagan Bansal
 *  ALL RIGHTS RESERVED.
 */

#include "../protocol/request.h"
#include "../core/kv_engine.h"
#include "../net/connection.h"

namespace kvmemo::server
{
    /**
     * @brief Holds runtime state for a single request execution.
     */
    class RequestContext final {
        public:
        RequestContext(net::Connection& connection, 
        const protocol::Request& request,
        core::KVEngine& engine) :
        connection_(connection),
        request_(request),
        engine_(engine) {}


        RequestContext(const RequestContext&) = delete;
        RequestContext& operator=(const RequestContext&) = delete;

        RequestContext(RequestContext&&) noexcept = default;
        RequestContext& operator=(RequestContext&&) noexcept = default;

        ~RequestContext() = default;

        /**
         * @brief Returns request object.
         */
        const protocol::Request& Request() const noexcept {
            return request_;
        }

        /**
         * @brief Returns KV engine.
         */
        core::KVEngine& Engine() noexcept {
            return engine_;
        }

        /**
         * @brief Returns network connection.
         */
        net::Connection& Connection() noexcept {
            return connection_;
        }

    private:    
        net::Connection& connection_;
        const protocol::Request& request_;
        core::KVEngine& engine_;
    };
} // namespace kvmemo::server

/**
 * This source code may not be copied, modified, or
 * distributed without explicit permission from the author.
 */