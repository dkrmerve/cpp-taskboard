#include <cstdlib>
#include <iostream>
#include <string>

#include <httplib.h>

#include "taskboard/api.hpp"
#include "taskboard/task_store.hpp"

namespace {

int env_int(const char* name, int fallback) {
    const char* value = std::getenv(name);
    if (value == nullptr || *value == '\0') return fallback;
    try {
        return std::stoi(value);
    } catch (...) {
        return fallback;
    }
}

std::string env_str(const char* name, const std::string& fallback) {
    const char* value = std::getenv(name);
    return (value == nullptr || *value == '\0') ? fallback : std::string(value);
}

}  // namespace

int main() {
    const int port = env_int("PORT", 8080);
    const std::string host = env_str("HOST", "0.0.0.0");
    const std::string static_dir = env_str("STATIC_DIR", "");

    taskboard::TaskStore store;
    httplib::Server server;
    taskboard::register_routes(server, store);

    if (!static_dir.empty()) {
        if (!server.set_mount_point("/", static_dir)) {
            std::cerr << "warning: cannot mount static dir " << static_dir << '\n';
        }
    }

    server.set_logger([](const httplib::Request& req, const httplib::Response& res) {
        std::cout << req.method << ' ' << req.path << " -> " << res.status << '\n';
    });

    std::cout << "taskboard " << TASKBOARD_VERSION << " listening on " << host << ':' << port
              << std::endl;
    if (!server.listen(host, port)) {
        std::cerr << "failed to bind " << host << ':' << port << '\n';
        return 1;
    }
    return 0;
}
