#include "inc/network/thread_pool.hpp" 
#include <atomic>
#include <chrono>
#include <iostream>
#include <thread>

int main() {
  std::cout << "=== Thread Pool Test ===" << std::endl;
  std::cout << "Creating thread pool with 2 workers, max_queue=2" << std::endl;

  network::ThreadPool pool(2, 2);
  std::atomic<int> active_count{0};
  std::atomic<int> completed_count{0};

  // Submit 10 tasks
  for (int i = 0; i < 10; ++i) {
    std::cout << "Submitting task " << i << std::endl;

    pool.submit([i, &active_count, &completed_count]() {
      active_count++;
      std::cout << "  Task " << i << " STARTED (active: " << active_count << ")"
                << std::endl;

      // Simulate work
      std::this_thread::sleep_for(std::chrono::seconds(2));

      active_count--;
      completed_count++;
      std::cout << "  Task " << i << " FINISHED (completed: " << completed_count
                << ")" << std::endl;
    });

    // Small delay to see output clearly
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  // Wait for all tasks to complete
  std::cout << "\nWaiting for all tasks to complete..." << std::endl;
  std::this_thread::sleep_for(std::chrono::seconds(25));

  std::cout << "\nCompleted: " << completed_count << " tasks" << std::endl;
  std::cout << "Active: " << active_count << " tasks" << std::endl;

  return 0;
}