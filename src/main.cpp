#include "inc/network/server.hpp"
#include "inc/kv_store.hpp"

int main(int argc, char* argv[]) {
    kvstore::KVStore store;
    std::string host = "127.0.0.1";
    int port = 6379;

    // Parse command-line arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--host" && i + 1 < argc) {
            host = argv[++i];
        } else if (arg == "--port" && i + 1 < argc) {
            port = std::atoi(argv[++i]);
        } else if (arg == "--help") {
            std::cout << "Usage: " << argv[0] << " [--host HOST] [--port PORT]\n";
            return 0;
        }
        else if(arg == "--version") {
            std::cout << "kvstore version 1.0.0\n";
            return 0;
        }
    }

    std::cout << "Starting kvstore server...\n";
    std::cout << "Host: " << host << ", Port: " << port << "\n";

    network::Server server(store, host, port);
    server.start();

    return 0;
}