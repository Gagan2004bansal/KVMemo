/**
 *  @file parser.h
 *  @brief Parse raw client input into KVMemo protocol requests.
 *
 *  Responsibilities :
 *  - Parse raw client command strings.
 *  - Tokenize command and arguments.
 *  - Construct Request objects.
 *
 *  Thread Safety :
 *  > Thread-Safe.
 *  > Stateless utility class.
 *
 *  Copyright © 2026
 *  Author: Gagan Bansal
 *  ALL RIGHTS RESERVED.
 */
#include <string>
#include <vector>
#include <sstream>
#include <stdexcept>

#include "request.h"

namespace kvmemo::protocol
{
    /**
     * @brief Parse client protocol commands into request objects.
     */
    class Parser final
    {
    public:
        Parser() = delete;
        ~Parser() = delete;

        Parser(const Parser &) = delete;
        Parser &operator=(const Parser &) = delete;

        /**
         * @brief Parse raw input string into Request.
         * Throws std::invalid_argument if command is empty.
         */
        static Request Parse(const std::string &input)
        {
            std::istringstream stream(input);

            std::vector<std::string> tokens;
            std::string token;

            while(stream >> token) {
                tokens.push_back(token);
            }

            if(tokens.empty()) {
                throw std::invalid_argument("Empty command");
            }

            std::string command = tokens.front();
            tokens.erase(tokens.begin());

            return Request(std::move(command), std::move(tokens));
        }
    };
} // namespace kvmemo::protocol

/**
 * This source code may not be copied, modified, or
 * distributed without explicit permission from the author.
 */