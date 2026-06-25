#include "inc/kv_store.hpp"
#include "inc/network/server.hpp"

int main(int argc, char *argv[]) {

  sigset_t mask;
  sigemptyset(&mask);
  sigaddset(&mask, SIGINT);
  sigaddset(&mask, SIGTERM);
  pthread_sigmask(SIG_BLOCK, &mask, nullptr);

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
    } else if (arg == "--version") {
      std::cout << "kvstore version 1.0.0\n";
      return 0;
    }
  }

  std::cout << "Starting kvstore server...\n";
  std::cout << "Host: " << host << ", Port: " << port << "\n";

  size_t pool_size = 2;
  size_t queue_size = 5;
  network::Server server(store, host, port, pool_size, queue_size);

  // Wait for signals directly in main thread
  server.start();
  int sig;
  sigwait(&mask, &sig); // ← Main thread blocks here
  std::cout << "\nSignal " << sig << " received, shutting down...\n";

  // server.wait_for_shutdown();
  server.stop();
  return 0;
}