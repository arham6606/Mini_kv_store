#include "inc/network/server.hpp"

namespace network {
namespace socket_utils {

int create_tcp_socket() {
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) {
    std::cerr << "ERROR: Failed to create socket\n";
    return -1;
  }

  // Allow reuse of address/port
  int opt = 1;
  if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
    std::cerr << "ERROR: Failed to set SO_REUSEADDR\n";
    close_socket(sockfd);
    return -1;
  }

  return sockfd;
}

bool bind_to_port(int sockfd, const std::string &host, int port) {
  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);

  // Convert host string to binary address
  if (inet_pton(AF_INET, host.c_str(), &addr.sin_addr) <= 0) {
    std::cerr << "ERROR: Invalid address: " << host << "\n";
    return false;
  }

  if (bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    std::cerr << "ERROR: Failed to bind to " << host << ":" << port << "\n";
    return false;
  }

  return true;
}

bool set_listen(int sockfd, int backlog) {
  if (listen(sockfd, backlog) < 0) {
    std::cerr << "ERROR: Failed to listen on socket\n";
    return false;
  }
  return true;
}

int accept_connection(int sockfd) {
  struct sockaddr_in client_addr;
  socklen_t addr_len = sizeof(client_addr);

  int client_fd = accept(sockfd, (struct sockaddr *)&client_addr, &addr_len);
  if (client_fd < 0) {
    std::cerr << "ERROR: Failed to accept connection\n";
    return -1;
  }

  std::cout << "Client connected from " << addr_to_string(client_addr) << "\n";
  return client_fd;
}

ssize_t recv_data(int sockfd, char *buffer, size_t len) {
  return recv(sockfd, buffer, len, 0);
}

bool send_data(int sockfd, const std::string &data) {
  ssize_t sent = send(sockfd, data.c_str(), data.size(), 0);
  if (sent < 0) {
    std::cerr << "ERROR: Failed to send data\n";
    return false;
  }
  return true;
}

void close_socket(int &sockfd) {
  if (sockfd >= 0) {
    close(sockfd);
    sockfd = -1;
  }
}

std::string addr_to_string(const struct sockaddr_in &addr) {
  char ip[INET_ADDRSTRLEN];
  inet_ntop(AF_INET, &addr.sin_addr, ip, INET_ADDRSTRLEN);
  return std::string(ip) + ":" + std::to_string(ntohs(addr.sin_port));
}

} // namespace socket_utils
} // namespace network