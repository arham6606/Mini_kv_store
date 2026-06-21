#include "kv_store.hpp"

namespace kvstore {

void KVStore::set(const std::string& key, const std::string& value) {
    store_[key] = value;
}

std::optional<std::string> KVStore::get(const std::string& key) const {
    auto it = store_.find(key);
    if (it == store_.end()) {
        return std::nullopt;
    }
    return it->second;  
}

bool KVStore::del(const std::string& key) {
  // this means that if the key is present in the database, it will be removed and the function will return true. If the key is not present, nothing will be removed and the function will return false.
    return store_.erase(key) > 0;
}

bool KVStore::exists(const std::string& key) const {
    return store_.find(key) != store_.end();
}

std::size_t KVStore::size() const noexcept {
    return store_.size();
}

}  // namespace kvstore