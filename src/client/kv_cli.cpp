#include "kv_client.h"

#include <iostream>
#include <string>

using namespace kvmemo::client;

/**
 * @brief Simple interactive CLI for KVMemo.
 *
 * This tool connects to a running KVMemo server
 * and allows users to send commands interactively.
 *
 * Example:
 *
 *   kvmemo> SET key value
 *   OK
 *
 *   kvmemo> GET key
 *   value
 */
int main(int argc, char* argv[])
{
    std::string host = "127.0.0.1";
    int port = 8082;

    if (argc >= 2) {
        host = argv[1];
    }

    if (argc >= 3) {
        port = std::stoi(argv[2]);
    }

    try {

        KVClient client(host, port);

        client.Connect();

        std::cout << "Connected to KVMemo server at "
                  << host << ":" << port << std::endl;

        std::cout << "Type 'exit' to quit." << std::endl;

        std::string line;

        while (true) {

            std::cout << "kvmemo> ";

            if (!std::getline(std::cin, line)) {
                break;
            }

            if (line == "exit" || line == "quit") {
                break;
            }

            if (line.empty()) {
                continue;
            }

            try {

                std::string response = client.SendCommand(line);

                std::cout << response;

                if (response.back() != '\n') {
                    std::cout << std::endl;
                }

            } catch (const std::exception& ex) {

                std::cerr << "Error: " << ex.what() << std::endl;

            }
        }

    } catch (const std::exception& ex) {

        std::cerr << "Connection failed: "
                  << ex.what() << std::endl;

        return 1;
    }

    return 0;
}