#pragma once

#include "inc/database/aof_persistence.hpp"
// #include "inc/network/server.hpp"

namespace network {
namespace socket_utils {
bool send_data(int sockfd, const std::string &data);
}// namespace socket_utiles
} // namespace network

#include <cctype>
#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <sstream>
#include <string>
#include <unordered_map>

namespace DataBase {
struct Entry {
  std::string value;
  std::optional<std::chrono::steady_clock::time_point> expiry;
  std::string expiry_str;
};
class KVStore {
public:
  KVStore() {
    store_.reserve(10000);
    store_.max_load_factor(0.3);
  }

  void set(const std::string &key, const std::string &value);

  // nodiscard just tells the caller that they should check the return value,
  // and store it that return value in a variable. If the caller ignores the
  // return value, the compiler will emit a warning.

  // Optional tells about the return value of the function. It can either
  // contain a string (the value associated with the key) or it can be empty (if
  // the key is not present in the store). This way, we can distinguish between
  // a key that is present with an empty string as its value, and a key that is
  // not present at all.

  [[nodiscard]] std::optional<std::string> get(int client_fd,
                                               const std::string &key);

  bool del(const std::string &key);

  [[nodiscard]] bool exists(const std::string &key) const;

  // noexcept tells the compiler that this function will not throw any
  // exceptions.

  [[nodiscard]] std::size_t size() const noexcept;

  // setter for aof persistence
  void set_write(const std::string &data);

  void start_up();
  void replay(const std::string &line);
  bool check_file_size();
  Entry entry_fields;

private:
  std::unordered_map<std::string, Entry> store_;
  DataBase::AOFPersistence aof_;
  mutable std::mutex mutex_;
};
} // namespace DataBase