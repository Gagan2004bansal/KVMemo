#pragma once
/**
 * @file serializer.h
 * @brief Serializes KVMemo protocol response into wire format.
 *
 *  Responsibilties :
 *  - Convert Response objects into network byte format.
 *  - Apply KVMemo wire protocol formatting.
 *  - Provide efficient serialization utilities.
 *
 * -------------------------------------------
 *  WIRE FORMAT :                             |
 *  - Simple String => +Ok\r\n                |
 *  - Bulk   String => $5\r\nAlice\r\n        |
 *  - Error         => -Err key not found\r\n |
 * -------------------------------------------
 *
 * Thread Safety :
 *  > Thread-Safe
 *  > Stateless utitlity class
 *
 *  Copyright © 2026
 *  Author: Gagan Bansal
 *  ALL RIGHTS RESERVED.
 */

#include <string>
#include <sstream>

#include "response.h"

namespace kvmemo::protocol
{
    /**
     * @brief Converts Response objects into protocol wire format.
     */
    class Serializer final
    {
    public:
        Serializer() = delete;
        ~Serializer() = delete;

        Serializer(const Serializer &) = delete;
        Serializer &operator=(const Serializer &) = delete;

        /**
         * @brief Serializes response into protocol string.
         */
        static std::string Serialize(const Response &response)
        {
            if (response.IsError())
            {
                return SerializeError(response.Message());
            }

            if (response.Message().empty())
            {
                return "+OK\r\n";
            }

            return SerializeBulkString(response.Message());
        }

    private:
        /**
         * @brief Serializes bulk string response.
         */
        static std::string SerializeBulkString(const std::string &value)
        {
            std::ostringstream ss;
            ss << "$" << value.size() << "\r\n";
            ss << value << "\r\n";

            return ss.str();
        }

        /**
         * @brief Serializes error response.
         */
        static std::string SerializeError(const std::string &message)
        {
            std::ostringstream ss;

            ss << "-ERR" << message << "\r\n";

            return ss.str();
        }
    };
} // namespace kvmemo::protocol

/**
 * This source code may not be copied, modified, or
 * distributed without explicit permission from the author.
 */