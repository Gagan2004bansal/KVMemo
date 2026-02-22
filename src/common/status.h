#pragma once
/**
 *  @file status.h
 *  @brief Lightweight status type for consistent error handling across KVMemo.
 *
 *  Copyright © 2026 Gagan Bansal
 *  ALL RIGHT RESERVED
 */

#include <string>
#include <utility>

namespace kvmemo::common
{

    /**
     *  @enum StatusCode
     *  @brief Enumerates common error categories used across the system.
     */

    enum class StatusCode : int
    {
        kOk = 0,

        // Generic failures
        kUnknown = 1,
        kInvalidArgument = 2,
        kNotFound = 3,
        kAlreadyExists = 4,
        kPermissionDenied = 5,

        // Networking / protocol
        kProtocolError = 100,
        kNetworkError = 101,
        kTimeout = 102,

        // Resource / system
        kResourceExhausted = 200,
        kInternalError = 201
    };

    /**
     *  @class Status
     *  @brief Represents the outcome of an operation.
     *
     *  Status is intentionally small and cheap to copy.
     *  A status is either :
     *  - OK (code == kOk)
     *  - Error (code != kOk, message contains details)
     */

    class Status final
    {
    public:
        /**
         *  @brief Creates an OK status
         */
        static Status Ok() { return Status(StatusCode::kOk, ""); }

        /**
         *  @brief Creates an error status with a specific code and message.
         *
         *  Prefer using the named helpers (InvalidArgument, NotFound, etc.)
         *  for readability.
         */

        static Status Error(StatusCode code, std::string message)
        {
            return Status(code, std::move(message));
        }

        static Status InvalidArgument(std::string message)
        {
            return Status(StatusCode::kInvalidArgument, std::move(message));
        }

        static Status NotFound(std::string message)
        {
            return Status(StatusCode::kNotFound, std::move(message));
        }

        static Status AlreadyExists(std::string message)
        {
            return Status(StatusCode::kAlreadyExists, std::move(message));
        }

        static Status PermissionDenied(std::string message)
        {
            return Status(StatusCode::kPermissionDenied, std::move(message));
        }

        static Status ProtocolError(std::string message)
        {
            return Status(StatusCode::kProtocolError, std::move(message));
        }

        static Status NetworkError(std::string message)
        {
            return Status(StatusCode::kNetworkError, std::move(message));
        }

        static Status Timeout(std::string message)
        {
            return Status(StatusCode::kTimeout, std::move(message));
        }

        static Status ResourceExhausted(std::string message)
        {
            return Status(StatusCode::kResourceExhausted, std::move(message));
        }

        static Status InternalError(std::string message)
        {
            return Status(StatusCode::kInternalError, std::move(message));
        }

        /**
         *  @brief Returns true if this status represents success.
         */
        [[nodiscard]] bool ok() const noexcept { return code_ == StatusCode::kOk; }

        /**
         *  @brief Returns the status code.
         *  For Ok status, message is empty.
         */
        [[nodiscard]] StatusCode code() const noexcept { return code_; }

        /**
         *  @brief Returns the human-readable message.
         *  For Ok status, message is empty.
         */
        [[nodiscard]] const std::string &message() const noexcept { return message_; }

        /**
         *  @brief Convenience method : returns message for logging/debugging.
         *
         *   Example : LOG_ERROR("SET failed : {}", status.ToString());
         */
        [[nodiscard]] std::string ToString() const
        {
            if (ok())
            {
                return "OK";
            }
            return CodeToString(code_) + ": " + message_;
        }

        /**
         * @brief Equality comparison (useful in tests).
         */
        friend bool operator==(const Status &a, const Status &b)
        {
            return a.code_ == b.code_ && a.message_ == b.message_;
        }

        friend bool operator!=(const Status &a, const Status &b) { return !(a == b); }

    private:
        Status(StatusCode code, std::string message) : code_(code), message_(std::move(message)) {}

        static std::string CodeToString(StatusCode code)
        {
            switch (code)
            {
            case StatusCode::kOk:
                return "OK";
            case StatusCode::kUnknown:
                return "UNKNOWN";
            case StatusCode::kInvalidArgument:
                return "INVALID_ARGUMENT";
            case StatusCode::kNotFound:
                return "NOT_FOUND";
            case StatusCode::kAlreadyExists:
                return "ALREADY_EXISTS";
            case StatusCode::kPermissionDenied:
                return "PERMISSION_DENIED";
            case StatusCode::kProtocolError:
                return "PROTOCOL_ERROR";
            case StatusCode::kNetworkError:
                return "NETWORK_ERROR";
            case StatusCode::kTimeout:
                return "TIMEOUT";
            case StatusCode::kResourceExhausted:
                return "RESOURCE_EXHAUSTED";
            case StatusCode::kInternalError:
                return "INTERNAL_ERROR";
            default:
                return "UNKNOWN";
            }
        }

        StatusCode code_;
        std::string message_;
    };
} // namespace kvmemo::common

/**
 * This source code may not be copied, modified, or
 * distributed without explicit permission from the author.
 */
