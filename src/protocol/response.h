#pragma once
/**
 * @file response.h
 * @brief Represents a server response in KVMemo protocol.
 *
 * Responsibilties :
 * - Represent the result of a command execution.
 * - Store response status (OK / ERROR).
 * - Store optional payload data returned to client.
 *
 * Thread Safety :
 * > Not thread-safe.
 * > Each response instance is owned by a single worker thread.
 *
 *  Copyright © 2026
 *  Author: Gagan Bansal
 *  ALL RIGHTS RESERVED.
 */

#include <string>
#include <utility>

namespace kvmemo::protocol
{
    /**
     * @brief Represents server response status.
     */
    enum class ResponseStatus
    {
        Ok,
        Error
    };

    /**
     * @brief Represents a protocol response sent to client.
     */
    class Response final
    {
    public:
        Response() = default;

        Response(ResponseStatus status,
                 std::string message = {})
            : status_(status),
              message_(std::move(message)) {}

        Response(const Response &) = default;
        Response &operator=(const Response &) = default;

        Response(Response &&) noexcept = default;
        Response &operator=(Response &&) noexcept = default;

        ~Response() = default;

        /**
         * @brief Creates success response.
         */
        static Response Ok(std::string message = {})
        {
            return Response(ResponseStatus::Ok, std::move(message));
        }

        /**
         * @brief Creates error response.
         */
        static Response Error(std::string message)
        {
            return Response(ResponseStatus::Error, std::move(message));
        }

        /**
         * @brief Returns response status.
         */
        ResponseStatus Status() const noexcept
        {
            return status_;
        }

        /**
         * @brief Returns response message payload.
         */
        const std::string &Message() const noexcept
        {
            return message_;
        }

        /**
         * @brief Check if response indicates success.
         */
        bool IsOk() const noexcept
        {
            return status_ == ResponseStatus::Ok;
        }

        /**
         * @brief Checks if response indicates error.
         */
        bool IsError() const noexcept
        {
            return status_ == ResponseStatus::Error;
        }

    private:
        ResponseStatus status_{ResponseStatus::Ok};
        std::string message_;
    };
} // namespace kvmemo::protocol

/**
 * This source code may not be copied, modified, or
 * distributed without explicit permission from the author.
 */