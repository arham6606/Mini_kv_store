#include "inc/database/kv_store.hpp"

namespace DataBase {

void KVStore::set(const std::string &key, const std::string &value) {
  std::unique_lock<std::mutex> lock(mutex_);
  auto expiry_time =
      std::chrono::steady_clock::now() + std::chrono::seconds(60);
  entry_fields.expiry_str =
      std::to_string(std::chrono::duration_cast<std::chrono::microseconds>(
                         expiry_time.time_since_epoch())
                         .count());
  store_[key] = {value, expiry_time};
}

std::optional<std::string> KVStore::get(int client_fd, const std::string &key) {
  std::unique_lock<std::mutex> lock(mutex_);
  auto it = store_.find(key);
  if (it == store_.end()) {
    return std::nullopt;
  }
   if (std::chrono::steady_clock::now() > it->second.expiry) {
   store_.erase(it);
   std::cout << "Key expired" << std::endl;
   network::socket_utils::send_data(client_fd, "Key expired");
   return std::nullopt;
   }
  return it->second.value;
}

bool KVStore::del(const std::string &key) {
  std::unique_lock<std::mutex> lock(mutex_);
  return store_.erase(key) > 0;
}

bool KVStore::exists(const std::string &key) const {
  std::unique_lock<std::mutex> lock(mutex_);
  return store_.find(key) != store_.end();
}

std::size_t KVStore::size() const noexcept {
  std::unique_lock<std::mutex> lock(mutex_);
  return store_.size();
}

void KVStore::set_write(const std::string &data) { aof_.write_data(data); }

void KVStore::start_up() {
  auto lines = aof_.read_all_data();

  for (const auto &line : lines) {
    replay(line);
  }
  std::cout << "Data backed up" << std::endl;
}

void KVStore::replay(const std::string &line) {
  std::istringstream iss(line);
  std::string cmd, key, value, expiration;
  iss >> cmd;
  iss >> key;
  if (cmd == "SET") {

    iss >> value;

    iss >> expiration;
    long long micro_seconds = std::stoll(expiration);
    auto expiry = std::chrono::steady_clock::time_point(
        std::chrono::microseconds(micro_seconds));

    store_[key] = {value, expiry};
  }
}

bool KVStore::check_file_size() { return aof_.file_empty(); }

KVStore::~KVStore() {
  stop_ = true;
  cv.notify_one();
  scanner_thread.join();
}

void KVStore::remove_expired() {
  auto now = std::chrono::steady_clock::now();
  std::unique_lock<std::mutex> lock(mutex_);

  for (auto it = store_.begin(); it != store_.end();) {
    if (it->second.expiry <= now)
      it = store_.erase(it); // erase returns next iterator
    else
      ++it;
  }
}

void KVStore::scanner_loop() {
  while (!stop_) {
    remove_expired();
    std::unique_lock<std::mutex> lock(cv_mutex);
    cv.wait_for(lock, std::chrono::seconds(5), [this] { return stop_.load(); });
  }

}

} // namespace DataBase