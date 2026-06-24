#pragma once

#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <sys/socket.h>
#include <thread>
#include <vector>

namespace network {

class ThreadPool {
private:
  std::queue<std::function<void()>> tasks_;
  std::mutex queue_mutex_;
  std::condition_variable cv_;
  std::atomic<bool> stop_{false};
  std::vector<std::thread> workers_;

public:
  ThreadPool(size_t pool_size);

  void submit(std::function<void()> task);

  void shutdown();

  ~ThreadPool();
};

} // namespace network