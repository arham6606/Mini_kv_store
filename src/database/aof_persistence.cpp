#include "inc/database/aof_persistence.hpp"

namespace DataBase {
AOFPersistence::AOFPersistence(const Config &config) : config_(config) {
  open();
}

void AOFPersistence::open() {
  config_.fd = ::open(config_.log_file_path.c_str(),
                      O_WRONLY | O_CREAT | O_APPEND, 0644);

  if (config_.fd == -1) {
    throw std::runtime_error("Failed to open: " + config_.log_file_path);
  }
}

void AOFPersistence::write_data(const std::string &data) {
  const char *ptr = data.c_str();
  size_t remaining = data.size();
  config_.is_file_empty = true;
  while (remaining > 0) {
    ssize_t written = ::write(config_.fd, ptr, remaining);
    if (written == -1) {
      if (errno == EINTR)
        continue; // interrupted, retry
      throw std::runtime_error("Write failed");
    }
    ptr += written;
    remaining -= written;
  }
  op_count_++;

  if (config_.auto_flush && op_count_ % config_.flush_every_n_ops == 0) {
    flush();
  }
}

void AOFPersistence::flush() {

  if (::fsync(config_.fd) == -1) {
    throw std::runtime_error("fsync failed");
  }
}

void AOFPersistence::close() {
  if (config_.fd != -1) {
    ::close(config_.fd);
    config_.fd = -1;
  }
}
AOFPersistence::~AOFPersistence() { close(); }

bool AOFPersistence::file_empty() { return config_.is_file_empty; }


std::vector<std::string> AOFPersistence::read_all_data() {
  int read_fd = ::open(config_.log_file_path.c_str(), O_RDONLY);
  if (read_fd == -1) {
    if (errno == ENOENT)
      return {}; // no log yet
    throw std::runtime_error("Failed to open for read");
  }

  off_t file_size = lseek(read_fd, 0, SEEK_END);
  if (file_size == -1) {
    ::close(read_fd);
    throw std::runtime_error("Failed to seek");
  }

  // FIX: Reset to beginning BEFORE reading
  if (lseek(read_fd, 0, SEEK_SET) == -1) {
    ::close(read_fd);
    throw std::runtime_error("Failed to seek to beginning");
  }

  if (file_size == 0) {
    ::close(read_fd);
    config_.is_file_empty = true;
    return {};
  }

  std::string buffer;
  buffer.reserve(file_size); // Optimization: pre-allocate
  char chunk[4096];
  ssize_t n;

  while ((n = ::read(read_fd, chunk, sizeof(chunk))) > 0) {
    buffer.append(chunk, n);
  }

  if (n == -1) { // FIX: Check for read error
    ::close(read_fd);
    throw std::runtime_error("Failed to read file");
  }

  ::close(read_fd);

  // Split into lines
  std::vector<std::string> lines;
  std::istringstream stream(buffer);
  std::string line;

  while (std::getline(stream, line)) {
    if (!line.empty()) {
      lines.push_back(line);
    }
  }

  return lines;
}
} // namespace DataBase