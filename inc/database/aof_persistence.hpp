#pragma once

#include <fcntl.h>
#include <functional>
#include <iostream>
#include <memory>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unistd.h>
#include <unordered_map>

namespace DataBase {

struct Config {
  std::string log_file_path = "data/appendonly.log";
  bool auto_flush = true;       // flush() after each write
  size_t flush_every_n_ops = 1; // flush every N operations
  int fd = -1;
  bool is_file_empty = false;
};
class AOFPersistence {
public:
  AOFPersistence(const Config &config = Config{});

  ~AOFPersistence();

  void open();
  void write_data(const std::string &data);
  void flush();
  void close();
  std::vector<std::string> read_all_data();
  bool file_empty();
private:
  Config config_;
  mutable std::mutex mutex_;
  size_t op_count_ = 0;
  size_t ops_since_flush_ = 0;
};
} // namespace DataBase