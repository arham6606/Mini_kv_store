#include "inc/network/server.hpp"
#include <cstring>

namespace network {

namespace {
    std::string to_upper(std::string s) {
        for (char& c : s) {
            c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        }
        return s;
    }
}

Server::Server(kvstore::KVStore& store, const std::string& host, int port)
    : store_(store), host_(host), port_(port), server_fd_(-1), running_(false) {
}

Server::~Server() {
    stop();
}

int Server::create_server_socket() {
    return socket_utils::create_tcp_socket();
}

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
    // 1. Create socket
    server_fd_ = create_server_socket();
    if (server_fd_ < 0) {
        return;
    }

    // 2. Bind
    if (!bind_socket(server_fd_)) {
        socket_utils::close_socket(server_fd_);
        return;
    }

    // 3. Listen
    if (!listen_socket(server_fd_)) {
        socket_utils::close_socket(server_fd_);
        return;
    }

    std::cout << "Server listening on " << host_ << ":" << port_ << "\n";
    running_ = true;

    // 4. Accept one client (for now)
    int client_fd = accept_client(server_fd_);
    if (client_fd < 0) {
        socket_utils::close_socket(server_fd_);
        return;
    }

    // 5. Handle client
    handle_client(client_fd);

    // 6. Cleanup
    socket_utils::close_socket(client_fd);
    socket_utils::close_socket(server_fd_);
    running_ = false;
    std::cout << "Server stopped\n";
}

void Server::stop() {
    running_ = false;
    if (server_fd_ >= 0) {
        socket_utils::close_socket(server_fd_);
    }
}

std::string Server::read_command(int client_fd) {
    static std::string buffer;  // Per-client buffer (but we only have one client)
    char chunk[1024];
    ssize_t bytes_received;

    bytes_received = socket_utils::recv_data(client_fd, chunk, sizeof(chunk) - 1);
    
    if (bytes_received <= 0) {
        if (bytes_received == 0) {
            std::cout << "Client disconnected\n";
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
        return "";  // Incomplete command, wait for more data
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

void Server::send_response(int client_fd, const std::string& response) {
    std::string formatted_response = response + "\n";
    socket_utils::send_data(client_fd, formatted_response);
}

std::string Server::process_command(const std::string& cmd_line) {
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
        if (!(iss >> key >> value)) {
            return "ERR wrong number of arguments";
        }
        // Check for extra tokens
        std::string extra;
        if (iss >> extra) {
            return "ERR wrong number of arguments";
        }
        store_.set(key, value);
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
        auto result = store_.get(key);
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
            ssize_t test_recv = recv(client_fd, &test_buf, 0, MSG_PEEK | MSG_DONTWAIT);
            if(test_recv == 0) {
                break;
            }
            
            if (test_recv < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
                break;
            }
            continue;
        }

        std::string response = process_command(command);
        
        if (response == "BYE") {
            send_response(client_fd, "Goodbye!");
            break;
        }
        
        send_response(client_fd, response);
    }
}

} // namespace network