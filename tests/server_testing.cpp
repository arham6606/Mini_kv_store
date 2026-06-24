#include <atomic>
#include <cassert>
#include <chrono>
#include <iostream>
#include <thread>
#include <vector>
#include "inc/kv_store.hpp"
class ConcurrentStressTest {
private:
  kvstore::KVStore store_;
  std::atomic<bool> stop_{false};
  std::atomic<int> success_count_{0};
  std::atomic<int> error_count_{0};

public:
  void run_writer(int thread_id, int operations) {
    for (int i = 0; i < operations && !stop_; ++i) {
      std::string key =
          "key_" + std::to_string(thread_id) + "_" + std::to_string(i);
      std::string value = "value_" + std::to_string(i);

      try {
        store_.set(key, value);
        success_count_++;

        // Occasionally delete
        if (i % 10 == 0) {
          store_.del(key);
        }
      } catch (...) {
        error_count_++;
        stop_ = true;
      }
    }
  }

  void run_reader(int thread_id, int operations) {
    for (int i = 0; i < operations && !stop_; ++i) {
      std::string key =
          "key_" + std::to_string(i % 10) + "_" + std::to_string(i);

      try {
        auto val = store_.get(key);
        if (val.has_value()) {
          // Successfully read
        }
        success_count_++;
      } catch (...) {
        error_count_++;
        stop_ = true;
      }
    }
  }

  void run_test(int num_writers, int num_readers, int ops_per_thread) {
    std::cout << "Starting stress test...\n";
    std::cout << "Writers: " << num_writers << ", Readers: " << num_readers
              << ", Ops/thread: " << ops_per_thread << "\n\n";

    std::vector<std::thread> threads;

    // Launch writer threads
    for (int i = 0; i < num_writers; ++i) {
      threads.emplace_back(&ConcurrentStressTest::run_writer, this, i,
                           ops_per_thread);
    }

    // Launch reader threads
    for (int i = 0; i < num_readers; ++i) {
      threads.emplace_back(&ConcurrentStressTest::run_reader, this, i,
                           ops_per_thread);
    }

    // Wait for all threads
    for (auto &t : threads) {
      t.join();
    }

    std::cout << "\n✅ Test completed!\n";
    std::cout << "Successful operations: " << success_count_ << "\n";
    std::cout << "Errors: " << error_count_ << "\n";
    assert(error_count_ == 0 && "Data race detected!");
  }
};

int main() {
  ConcurrentStressTest test;

  // Run with 4 writers, 4 readers, 10000 operations each
  test.run_test(4, 4, 10000);

  return 0;
}