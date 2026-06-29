#pragma once

#include "inc/database/kv_store.hpp"
#include "inc/network/thread_pool.hpp"

#include <arpa/inet.h>
#include <atomic>
#include <cctype>
#include <cerrno>
#include <csignal>
#include <cstring>
#include <fcntl.h>
#include <functional>
#include <iostream>
#include <memory>
#include <netinet/in.h>
#include <sstream>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>
#include <vector>

namespace network {
class Server {
public:
  Server(DataBase::KVStore &store, const std::string &host = "127.0.0.1",
         int port = 6379, size_t pool_size = 4, size_t queue_size = 100);

  ~Server();

  // Non-copyable
  Server(const Server &) = delete;
  Server &operator=(const Server &) = delete;

  // Start the server (blocks until shutdown)
  void start();

  // Stop the server
  void stop();

private:
  // Socket operations
  int create_server_socket();
  bool bind_socket(int sockfd);
  bool listen_socket(int sockfd);
  int accept_client(int sockfd);
  void send_response(int client_fd, const std::string &response);

  // Client handling
  void handle_client(int client_fd);
  std::string read_command(int client_fd);
  std::string parse_quoted(std::istringstream&iss);
  // Command processing
  std::string process_command(int client_fd, const std::string &cmd);

  int port_;
  int server_fd_;
  std::atomic<bool> running_{false};
  DataBase::KVStore &store_;
  std::string host_;
  std::vector<int> client_fds_;
  std::mutex client_fds_mutex_;
  std::mutex shutdown_mutex_;
  std::condition_variable shutdown_cv_;
  std::thread accept_thread_;
  ThreadPool pool;
};

namespace socket_utils {

int create_tcp_socket();

bool bind_to_port(int sockfd, const std::string &host, int port);

bool set_listen(int sockfd, int backlog = 5);

int accept_connection(int sockfd);

ssize_t recv_data(int sockfd, char *buffer, size_t len);

bool send_data(int sockfd, const std::string &data);

void close_socket(int &sockfd);

std::string addr_to_string(const struct sockaddr_in &addr);

} // namespace socket_utils

} // namespace network