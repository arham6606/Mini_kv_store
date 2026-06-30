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

  if (op_count_ > SNAPSHOT_INTERVEL) {
    take_snapshot();
    op_count_ = 0;
  }
}

void AOFPersistence::take_snapshot() {
  std::cout << "📸 Taking snapshot..." << std::endl;

  // Step 1: Read all entries from current log
  std::unordered_map<std::string, std::string> latest_values;

  // Re-open log for reading
  int log_fd = ::open(config_.log_file_path.c_str(), O_RDONLY);
  if (log_fd == -1) {
    throw std::runtime_error("Failed to open log for snapshot");
  }

  // Read and parse each line
  std::string line;
  char buffer[1024];
  ssize_t bytes_read;
  std::string remaining_data;

  while ((bytes_read = ::read(log_fd, buffer, sizeof(buffer) - 1)) > 0) {
    buffer[bytes_read] = '\0';
    std::string chunk(buffer, bytes_read);

    // Split by newlines
    size_t pos = 0;
    std::string combined = remaining_data + chunk;

    while ((pos = combined.find('\n')) != std::string::npos) {
      line = combined.substr(0, pos);
      combined.erase(0, pos + 1);

      if (!line.empty()) {
        // Parse: "SET key value"
        std::istringstream iss(line);
        std::string cmd, key, value;
        iss >> cmd >> key;

        // Get the rest as value (supports spaces)
        std::getline(iss, value);
        if (!value.empty() && value[0] == ' ') {
          value.erase(0, 1); // Remove leading space
        }

        if (cmd == "SET") {
          latest_values[key] = value; // Overwrites previous! ✅
        }
      }
    }

    remaining_data = combined;
  }

  ::close(log_fd);

  // Step 2: Write latest values to snapshot.tmp
  std::string temp_path = config_.snapshot_path;
  int snapshot_fd =
      ::open(temp_path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
  if (snapshot_fd == -1) {
    throw std::runtime_error("Failed to create snapshot temp file");
  }

  // Write each key-value pair
  for (const auto &[key, value] : latest_values) {
    std::string line = "SET " + key + " " + value + "\n";
    const char *ptr = line.c_str();
    size_t remaining = line.size();

    while (remaining > 0) {
      ssize_t written = ::write(snapshot_fd, ptr, remaining);
      if (written == -1) {
        if (errno == EINTR)
          continue;
        throw std::runtime_error("Failed to write snapshot");
      }
      ptr += written;
      remaining -= written;
    }
  }

  // Flush and close snapshot
  ::fsync(snapshot_fd);
  ::close(snapshot_fd);

  // Step 3: Atomic rename
  if (::rename(temp_path.c_str(), "data/snapshot.db") != 0) {
    throw std::runtime_error("Failed to rename snapshot");
  }

  // Step 4: Clear log (COMPACTION!)
  clear_log();

  std::cout << "✅ Snapshot taken with " << latest_values.size()
            << " unique keys" << std::endl;
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
  int read_fd = ::open(config_.snapshot_path.c_str(), O_RDONLY);
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

void AOFPersistence::clear_log() { ftruncate(config_.fd, 0); }
} // namespace DataBase