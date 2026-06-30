#include <cstring>

#include "inc/network/server.hpp"

namespace network {

namespace {
std::string to_upper(std::string s) {
  for (char &c : s) {
    c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
  }
  return s;
}
} // namespace

Server::Server(DataBase::KVStore &store, const std::string &host, int port,
               size_t pool_size, size_t max_queue)
    : store_(store), host_(host), port_(port), pool(pool_size, max_queue),
      server_fd_(-1), running_(false) {}

Server::~Server() {
  if (running_) {
    stop();
  }
}

int Server::create_server_socket() { return socket_utils::create_tcp_socket(); }

bool Server::bind_socket(int sockfd) {
  return socket_utils::bind_to_port(sockfd, host_, port_);
}

bool Server::listen_socket(int sockfd) {
  return socket_utils::set_listen(sockfd, 5);
}

int Server::accept_client(int sockfd) {
  return socket_utils::accept_connection(sockfd);
}

void Server::start() {

  server_fd_ = create_server_socket();
  if (server_fd_ < 0) {
    return;
  }

  if (!bind_socket(server_fd_)) {
    socket_utils::close_socket(server_fd_);
    return;
  }

  if (!listen_socket(server_fd_)) {
    socket_utils::close_socket(server_fd_);
    return;
  }

  std::cout << "Server listening on " << host_ << ":" << port_ << "\n";
  std::cout << "Press Ctrl+C to shutdown gracefully\n\n";

  running_.store(true);
  accept_thread_ = std::thread([this]() {
    while (running_.load()) {
      int client_fd = accept_client(server_fd_);
      if (client_fd < 0) {
        if (errno == EINTR) {
          continue;
        }
        continue;
      }

      // Thread-safe push
      {
        std::lock_guard<std::mutex> lock(client_fds_mutex_);
        client_fds_.push_back(client_fd);
      }

      pool.submit([this, client_fd]() { handle_client(client_fd); });
    }
  });
}

void Server::stop() {
  // std::cout << "\nShutting down server...\n";

  // 1. Stop accepting new connections
  running_.store(false);

  // 2. Interrupt accept() to wake up the thread
  if (server_fd_ >= 0) {
    shutdown(server_fd_, SHUT_RDWR);
    socket_utils::close_socket(server_fd_);
    server_fd_ = -1;
  }

  // 3. Wait for accept thread to finish
  if (accept_thread_.joinable()) {
    accept_thread_.join();
  }

  // 4. Close all client sockets (thread-safe)

  {
    std::lock_guard<std::mutex> lock(client_fds_mutex_);
    if (!client_fds_.empty()) {
      std::cout << "   Closing client connections (" << client_fds_.size()
                << ")...\n";
      for (int fd : client_fds_) {
        shutdown(fd, SHUT_RDWR);
        socket_utils::close_socket(fd);
      }
      client_fds_.clear();
    }
  }
  pool.shutdown();

  std::cout << "Server stopped cleanly\n";
}

std::string Server::read_command(int client_fd) {
  static thread_local std::string buffer;
  char chunk[1024];
  ssize_t bytes_received;

  bytes_received = socket_utils::recv_data(client_fd, chunk, sizeof(chunk) - 1);

  if (bytes_received <= 0) {
    if (bytes_received == 0) {
      std::cout << "Client Disconnected\n";
    } else {
      std::cerr << "ERROR: recv failed\n";
    }
    return "";
  }

  chunk[bytes_received] = '\0';
  buffer += chunk;

  // Look for newline
  size_t pos = buffer.find('\n');
  if (pos == std::string::npos) {
    return ""; // Incomplete command, wait for more data
  }

  // Extract the complete line
  std::string command = buffer.substr(0, pos);
  buffer.erase(0, pos + 1);

  // Trim whitespace
  while (!command.empty() && std::isspace(command.back())) {
    command.pop_back();
  }

  return command;
}

void Server::send_response(int client_fd, const std::string &response) {
  std::string formatted_response = response + "\n";
  socket_utils::send_data(client_fd, formatted_response);
}
std::string Server::parse_quoted(std::istringstream &iss) {
  std::string token;
  if (!(iss >> token)) {
    return "";
  }

  // Case 1: Starts with quote
  if (token.front() == '"' || token.front() == '\'') {
    char quote = token.front();

    // Single token with matching quotes: "arham" or 'arham'
    if (token.length() > 1 && token.back() == quote) {
      return token.substr(1, token.length() - 2);
    }

    // Multi-word quoted string
    std::string full = token;
    while (token.back() != quote) {
      if (!(iss >> token)) {
        return ""; // Unclosed opening quote → ERROR
      }
      full += " " + token;
    }
    return full.substr(1, full.length() - 2);
  }

  // Case 2: Doesn't start with quote
  // Check if ANY remaining token has a stray closing quote
  std::string full = token;
  std::string next;

  while (iss >> next) {
    if (next.back() == '"' || next.back() == '\'') {
      return ""; // Stray closing quote → ERROR
    }
    full += " " + next;
  }
  if (full.back() == '"') {
    full.pop_back();
  }

  return full;
}

std::string Server::process_command(int client_fd,
                                    const std::string &cmd_line) {
  std::istringstream iss(cmd_line);
  std::string cmd;
  iss >> cmd;

  if (cmd.empty()) {
    return "";
  }

  cmd = to_upper(cmd);

  if (cmd == "PING") {
    return "PONG";

  } else if (cmd == "SET") {
    std::string key, value;
    if (!(iss >> key)) {
      return "ERR wrong number of arguments";
    }

    value = parse_quoted(iss);
    if (value.empty()) {
      return "ERR wrong number of arguments";
    }

    store_.set(key, value);
    if (!store_.check_file_size()) {
      store_.set_write("SET " + key + " " + value + " " +
                       store_.entry_fields.expiry_str + "\n");
    } else {
      store_.set_write("SET " + key + " " + value + " " +
                       store_.entry_fields.expiry_str + "\n");
    }
    return "OK";

  } else if (cmd == "GET") {
    std::string key;
    if (!(iss >> key)) {
      return "ERR wrong number of arguments";
    }
    std::string extra;
    if (iss >> extra) {
      return "ERR wrong number of arguments";
    }
    auto result = store_.get(client_fd, key);
    if (result.has_value()) {
      return *result;
    }
    return "(nil)";

  } else if (cmd == "DEL") {
    std::string key;
    if (!(iss >> key)) {
      return "ERR wrong number of arguments";
    }
    std::string extra;
    if (iss >> extra) {
      return "ERR wrong number of arguments";
    }
    return store_.del(key) ? "1" : "0";

  } else if (cmd == "EXIT" || cmd == "QUIT") {
    return "BYE";

  } else {
    return "ERR unknown command '" + cmd + "'";
  }
}

void Server::handle_client(int client_fd) {
  std::cout << "Handling client...\n";

  while (running_) {
    std::string command = read_command(client_fd);

    if (command.empty()) {
      // Check if client disconnected
      // We'll detect this in read_command by checking bytes_received
      // For simplicity, if command is empty, break
      // But we need to handle incomplete commands differently

      char test_buf;
      ssize_t test_recv =
          recv(client_fd, &test_buf, 0, MSG_PEEK | MSG_DONTWAIT);
      if (test_recv == 0) {
        break;
      }

      if (test_recv < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
        break;
      }
      continue;
    }

    std::string response = process_command(client_fd, command);

    if (response == "BYE") {
      send_response(client_fd, "Goodbye!");
      std::cout << "Client Disconnected" << std::endl;
      break;
    } else {
      send_response(client_fd, response);
    }
  }
}

} // namespace network