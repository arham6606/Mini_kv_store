#include <iostream>
#include <string>

#include  "inc/database/kv_store.hpp"

namespace {

int g_checks = 0;
int g_failures = 0;

void check(bool condition, const std::string& description) {
    ++g_checks;
    if (condition) {
        std::cout << "[PASS] " << description << "\n";
    } else {
        ++g_failures;
        std::cout << "[FAIL] " << description << "\n";
    }
}

}  // namespace

int main() {
    DataBase::KVStore store;

    check(store.size() == 0, "new store has size 0");

    // SET name arham
    store.set("name", "arham");
    check(store.size() == 1, "size is 1 after one SET");

    // GET name -> arham
    {
        auto v = store.get("name");
        check(v.has_value() && *v == "arham", "GET name -> arham");
    }

    // A key that was never set
    check(!store.exists("ghost"), "EXISTS ghost -> false (never set)");
    check(!store.get("ghost").has_value(), "GET ghost -> nullopt (never set)");

    // DEL name -> true
    check(store.del("name") == true, "DEL name -> true");
    check(store.size() == 0, "size is 0 after DEL");

    // GET name -> missing
    check(!store.get("name").has_value(), "GET name -> missing after DEL");

    // Deleting an already-missing key
    check(store.del("name") == false, "DEL name again -> false (already gone)");

    // SET empty ""
    store.set("empty", "");

    // GET empty -> ""   (must be distinguishable from "missing")
    {
        auto v = store.get("empty");
        check(v.has_value(), "GET empty -> has_value() true (key exists)");
        check(v.has_value() && *v == "", "GET empty -> value is \"\"");
    }
    check(store.exists("empty"), "EXISTS empty -> true even though value is \"\"");

    // Overwrite semantics
    store.set("name", "first");
    store.set("name", "second");
    {
        auto v = store.get("name");
        check(v.has_value() && *v == "second", "SET on existing key overwrites value");
    }
    check(store.size() == 2, "size counts distinct keys only (name, empty)");

    std::cout << "\n" << (g_checks - g_failures) << "/" << g_checks << " checks passed\n";
    return g_failures == 0 ? 0 : 1;
}