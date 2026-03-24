#pragma once
/**
 * @file dispatcher.h
 * @brief Routes protocol requests to KVEngine operations.
 *
 * Responsibilities :
 * - Interpret parsed client requests.
 * - Map commands to KVEngine operations.
 * - Produce protocol responses.
 *
 * Thread Safety :
 *  > Thread-safe if KVEngine is thread-safe.
 *  > Stateless dispatcher.
 *
 *  Copyright © 2026 KVMemo
 *  Author: Gagan Bansal
 *  ALL RIGHTS RESERVED.
 */

#include <string>
#include <optional>

#include "../protocol/request.h"
#include "../protocol/response.h"
#include "../core/kv_engine.h"

namespace kvmemo::server
{
    class Dispatcher final
    {
    public:
        /**
         * @brief Constructs dispatcher with KV engine dependency.
         */
        explicit Dispatcher(core::KVEngine &engine) : engine_(engine) {}

        Dispatcher(const Dispatcher &) = delete;
        Dispatcher &operator=(const Dispatcher &) = delete;

        Dispatcher(Dispatcher &&) noexcept = default;
        Dispatcher &operator=(Dispatcher &&) noexcept = delete;

        ~Dispatcher() = default;

        /**
         * @brief Dispatch request to correct engine command.
         */
        protocol::Response Dispatch(const protocol::Request &request)
        {
            if (request.Empty())
            {
                return protocol::Response::Error("Empty Command");
            }

            const std::string &cmd = request.Command();

            if (cmd == "SET")
            {
                return HandleSet(request);
            }

            if (cmd == "GET")
            {
                return HandleGet(request);
            }

            if (cmd == "DEL")
            {
                return HandleDelete(request);
            }

            if (cmd == "KEYS")
            {
                return HandleKeys(request);
            }

            return protocol::Response::Error("Unknown command");
        }

    private:
        protocol::Response HandleSet(const protocol::Request &req)
        {
            if (req.ArgCount() < 2)
            {
                return protocol::Response::Error("SET requires key and value");
            }

            const std::string &key = req.Arg(0);
            const std::string &value = req.Arg(1);

            engine_.Set(key, value);

            return protocol::Response::Ok();
        }

        protocol::Response HandleGet(const protocol::Request &req)
        {
            if (req.ArgCount() < 1)
            {
                return protocol::Response::Error("Get requires key");
            }

            const std::string &key = req.Arg(0);

            auto value = engine_.Get(key);

            if (!value.has_value())
            {
                return protocol::Response::Error("Key not found");
            }

            return protocol::Response::Ok(value.value());
        }

        protocol::Response HandleDelete(const protocol::Request &req)
        {
            if (req.ArgCount() < 1)
            {
                return protocol::Response::Error("DEL requires key");
            }

            const std::string &key = req.Arg(0);

            engine_.Delete(key);

            return protocol::Response::Ok();
        }

        /**
         * @brief Handles the KEYS command — returns all key:value pairs.
         */
        protocol::Response HandleKeys(const protocol::Request &req)
        {
            if (req.ArgCount() > 0)
            {
                return protocol::Response::Error("KEYS takes no arguments");
            }

            auto pairs = engine_.GetAllKeys();

            std::string result;
            for (const auto &[key, value] : pairs)
            {
                result += key + ":" + value + "\n";
            }

            // Remove trailing newline if present
            if (!result.empty() && result.back() == '\n')
            {
                result.pop_back();
            }

            return protocol::Response::Ok(result);
        }

    private:
        core::KVEngine &engine_;
    };
} // namespace kvmemo::server

/**
 * This source code may not be copied, modified, or
 * distributed without explicit permission from the author.
 */