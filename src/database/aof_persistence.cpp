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

} // namespace DataBase
