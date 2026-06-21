#include <cctype>
#include <sstream>
#include "kv_store.hpp"

namespace {

// Strips one surrounding pair of double quotes, if present, so a user can
// type `SET empty ""` to explicitly mean "store the empty string" rather
// than leaving the value ambiguous.

std::string strip_quotes(std::string s) {
    if (s.size() >= 2 && s.front() == '"' && s.back() == '"') {
        return s.substr(1, s.size() - 2);
    }
    return s;
}

std::string to_upper(std::string s) {
    for (char& c : s) {
        c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    }
    return s;
}

void print_help() {
    std::cout <<
        "Commands:\n"
        "  SET <key> <value>   set key to value (use \"\" for an empty value)\n"
        "  GET <key>           print the value, or (nil) if missing\n"
        "  DEL <key>           remove key, prints (1) if removed, (0) if absent\n"
        "  EXISTS <key>        prints (1) or (0)\n"
        "  SIZE                print number of keys currently stored\n"
        "  HELP                show this message\n"
        "  EXIT / QUIT         leave the REPL\n";
}

}  // namespace

int main() {
    kvstore::KVStore store;
    std::string line;

    std::cout << "kvstore (stage 1: in-memory, no networking). Type HELP.\n";

    while (true) {
        std::cout << "> ";
        if (!std::getline(std::cin, line)) {
            std::cout << "\n";
            break;  // EOF, e.g. Ctrl-D
        }

        std::istringstream iss(line);
        std::string cmd;
        iss >> cmd;
        if (cmd.empty()) {
            continue;
        }
        cmd = to_upper(cmd);

        if (cmd == "EXIT" || cmd == "QUIT") {
            break;

        } else if (cmd == "HELP") {
            print_help();

        } else if (cmd == "SIZE") {
            std::cout << store.size() << "\n";

        } else if (cmd == "SET") {
            std::string key;
            iss >> key;
            if (key.empty()) {
                std::cout << "(error) usage: SET <key> <value>\n";
                continue;
      
            }
            std::string value;
            std::getline(iss, value);
            if (!value.empty() && value.front() == ' ') {
                value.erase(0, 1);  // drop the single separating space
            }
            store.set(key, strip_quotes(value));
            std::cout << "OK\n";

        } else if (cmd == "GET") {
            std::string key;
            iss >> key;
            if (key.empty()) {
                std::cout << "(error) usage: GET <key>\n";
                continue;
            }
            if (auto result = store.get(key); result.has_value()) {
                std::cout << "\"" << *result << "\"\n";
            } else {
                std::cout << "(nil)\n";
            }

        } else if (cmd == "DEL") {
            std::string key;
            iss >> key;
            if (key.empty()) {
                std::cout << "(error) usage: DEL <key>\n";
                continue;
            }
            std::cout << "(" << (store.del(key) ? 1 : 0) << ")\n";

        } else if (cmd == "EXISTS") {
            std::string key;
            iss >> key;
            if (key.empty()) {
                std::cout << "(error) usage: EXISTS <key>\n";
                continue;
            }
            std::cout << "(" << (store.exists(key) ? 1 : 0) << ")\n";

        }
        
        else if(cmd == "PING")
        {
            std::cout << "PONG\n";
        }
        
        else {
            std::cout << "(error) unknown command '" << cmd << "'. Type HELP.\n";
        }
    }

    return 0;
}