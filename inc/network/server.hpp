#pragma once


#include <cstring>
#include <iostream>
#include <memory>
#include <functional>
#include "inc/kv_store.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sstream>
#include <cctype>
#include <cstring>
namespace network {

class Server {
public:
    Server(kvstore::KVStore& store, const std::string& host = "127.0.0.1", int port = 6379);
    ~Server();

    // Non-copyable
    Server(const Server&) = delete;
    Server& operator=(const Server&) = delete;

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
    
    // Client handling
    void handle_client(int client_fd);
    std::string read_command(int client_fd);
    void send_response(int client_fd, const std::string& response);
    
    // Command processing
    std::string process_command(const std::string& cmd);

    kvstore::KVStore& store_;
    std::string host_;
    int port_;
    int server_fd_;
    bool running_;
};

namespace socket_utils {
  
int create_tcp_socket();

bool bind_to_port(int sockfd, const std::string& host, int port);

bool set_listen(int sockfd, int backlog = 5);

int accept_connection(int sockfd);

ssize_t recv_data(int sockfd, char* buffer, size_t len);

bool send_data(int sockfd, const std::string& data);

void close_socket(int& sockfd);

std::string addr_to_string(const struct sockaddr_in& addr);

} // namespace socket_utils

} // namespace network