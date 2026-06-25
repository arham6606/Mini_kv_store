#include "inc/network/thread_pool.hpp"

namespace network {

ThreadPool::ThreadPool(size_t num_threads, size_t queue_size)
    : max_queue(queue_size) {

  for (size_t i = 0; i < num_threads; ++i) {
    workers_.emplace_back([this]() {
      while (true) {
        std::function<void()> task;
        {
          std::unique_lock<std::mutex> lock(queue_mutex_);
          cv_.wait(lock, [this]() { return stop_ || !tasks_.empty(); });
          if (stop_ && tasks_.empty())
            return;
          task = std::move(tasks_.front());
          tasks_.pop();
          cv_.notify_all();
        }
        task(); // Execute database work here
      }
    });
  }
}

void ThreadPool::submit(std::function<void()> task) {
  std::unique_lock<std::mutex> lock(queue_mutex_);
  // Wait if queue is full (bounded queue)
  cv_.wait(lock, [this]() { return stop_ || tasks_.size() < max_queue; });

  // Don't accept new tasks during shutdown
  if (stop_)
    return;

  tasks_.push(std::move(task));
  std::cout << tasks_.size() << std::endl;
  cv_.notify_one();
}

void ThreadPool::shutdown() {
  stop_ = true;
  cv_.notify_all();
  for (auto &w : workers_) {
    if (w.joinable())
      w.join();
  }
}

ThreadPool::~ThreadPool() { shutdown(); }

} // namespace network