#include "taskboard/api.hpp"

#include <charconv>
#include <optional>
#include <stdexcept>
#include <string>

#include <nlohmann/json.hpp>

namespace taskboard {
namespace {

using nlohmann::json;

void send_json(httplib::Response& res, int status, const json& body) {
    res.status = status;
    res.set_content(body.dump(), "application/json");
}

void send_error(httplib::Response& res, int status, const std::string& message) {
    send_json(res, status, json{{"error", message}});
}

std::optional<std::uint64_t> parse_id(const std::string& text) {
    std::uint64_t value = 0;
    const auto* begin = text.data();
    const auto* end = begin + text.size();
    const auto [ptr, ec] = std::from_chars(begin, end, value);
    if (ec != std::errc{} || ptr != end) return std::nullopt;
    return value;
}

std::optional<json> parse_body(const httplib::Request& req, httplib::Response& res) {
    if (req.body.empty()) {
        send_error(res, 400, "request body is required");
        return std::nullopt;
    }
    auto parsed = json::parse(req.body, nullptr, false);
    if (parsed.is_discarded()) {
        send_error(res, 400, "invalid JSON");
        return std::nullopt;
    }
    return parsed;
}

}  // namespace

void register_routes(httplib::Server& server, TaskStore& store) {
    // CORS so the frontend can be served from a different origin during development.
    server.set_default_headers({
        {"Access-Control-Allow-Origin", "*"},
        {"Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS"},
        {"Access-Control-Allow-Headers", "Content-Type"},
    });
    server.Options(R"(/api/.*)", [](const httplib::Request&, httplib::Response& res) {
        res.status = 204;
    });

    server.Get("/api/health", [](const httplib::Request&, httplib::Response& res) {
        send_json(res, 200, json{{"status", "ok"}, {"version", TASKBOARD_VERSION}});
    });

    server.Get("/api/stats", [&store](const httplib::Request&, httplib::Response& res) {
        const Stats s = store.stats();
        send_json(res, 200, json{
            {"total", s.total},
            {"todo", s.todo},
            {"in_progress", s.in_progress},
            {"done", s.done},
        });
    });

    server.Get("/api/tasks", [&store](const httplib::Request& req, httplib::Response& res) {
        std::optional<Status> filter;
        if (req.has_param("status")) {
            filter = status_from_string(req.get_param_value("status"));
            if (!filter) {
                send_error(res, 400, "unknown status filter");
                return;
            }
        }
        send_json(res, 200, json(store.list(filter)));
    });

    server.Post("/api/tasks", [&store](const httplib::Request& req, httplib::Response& res) {
        const auto body = parse_body(req, res);
        if (!body) return;
        try {
            const Task task = store.create(parse_patch(*body));
            send_json(res, 201, json(task));
        } catch (const std::invalid_argument& e) {
            send_error(res, 400, e.what());
        }
    });

    // Bulk delete: DELETE /api/tasks?status=done
    server.Delete("/api/tasks", [&store](const httplib::Request& req, httplib::Response& res) {
        if (!req.has_param("status")) {
            send_error(res, 400, "status query parameter is required");
            return;
        }
        const auto status = status_from_string(req.get_param_value("status"));
        if (!status) {
            send_error(res, 400, "unknown status filter");
            return;
        }
        send_json(res, 200, json{{"removed", store.remove_by_status(*status)}});
    });

    server.Get(R"(/api/tasks/(\d+))", [&store](const httplib::Request& req, httplib::Response& res) {
        const auto id = parse_id(req.matches[1]);
        if (!id) {
            send_error(res, 400, "invalid id");
            return;
        }
        const auto task = store.get(*id);
        if (!task) {
            send_error(res, 404, "task not found");
            return;
        }
        send_json(res, 200, json(*task));
    });

    server.Put(R"(/api/tasks/(\d+))", [&store](const httplib::Request& req, httplib::Response& res) {
        const auto id = parse_id(req.matches[1]);
        if (!id) {
            send_error(res, 400, "invalid id");
            return;
        }
        const auto body = parse_body(req, res);
        if (!body) return;
        try {
            const auto task = store.update(*id, parse_patch(*body));
            if (!task) {
                send_error(res, 404, "task not found");
                return;
            }
            send_json(res, 200, json(*task));
        } catch (const std::invalid_argument& e) {
            send_error(res, 400, e.what());
        }
    });

    server.Delete(R"(/api/tasks/(\d+))", [&store](const httplib::Request& req, httplib::Response& res) {
        const auto id = parse_id(req.matches[1]);
        if (!id) {
            send_error(res, 400, "invalid id");
            return;
        }
        if (!store.remove(*id)) {
            send_error(res, 404, "task not found");
            return;
        }
        res.status = 204;
    });
}

}  // namespace taskboard
