#pragma once 
/**
 * @file request.h
 * @brief Represents a parsed client request in KVMemo protocol.
 * 
 * Responsibilties :
 * - Represent a single client command.
 * - Store command arguments.
 * - Provide read-only access to parsed tokens.
 * 
 *  Thread Safety :
 *  > Not Thread-safe.
 *  > Each request instance is owned by a single worker thread.
 *
 *  Copyright © 2026
 *  Author: Gagan Bansal
 *  ALL RIGHTS RESERVED.  
 */

#include <string>
#include <vector>
#include <stdexcept>

namespace kvmemo::protocol
{
    /**
     * @brief Represents a parsed client command.
     */
    class Request final {
        public:
        Request() = default;

        Request(std::string command, std::vector<std::string> args)
        : command_(std::move(command)),
        args_(std::move(args)) {}

        Request(const Request&) = default;
        Request& operator=(const Request&) = default;

        Request(Request&&) noexcept = default;
        Request& operator=(Request&&) noexcept = default;

        ~Request() = default;

        /**
         * @brief Returns command name.
         */
        const std::string& Command() const noexcept {
            return command_;
        }

        /**
         * @brief Returns number of arguments.
         */
        std::size_t ArgCount() const noexcept {
            return args_.size();
        }

        /**
         * @brief Returns argument at index.
         * Throws std::out_of_range if index invalid.
         */
        const std::string& Arg(std::size_t index) const {
            if(index >= args_.size()) {
                throw std::out_of_range("Request argument index out range");
            }
            return args_[index];
        }

        /**
         * @brief Returns all arguments.
         */
        const std::vector<std::string>& Args() const noexcept {
            return args_;
        }

        /**
         * @brief Checks if request is empty.
         */
        bool Empty() const noexcept {
            return command_.empty();
        }

    private:
        std::string command_;
        std::vector<std::string> args_;
    };
} // namespace kvmemo::protocol


/**
 * This source code may not be copied, modified, or
 * distributed without explicit permission from the author.
 */