#include "taskboard/task.hpp"

#include <chrono>
#include <ctime>
#include <stdexcept>

namespace taskboard {

std::string to_string(Status status) {
    switch (status) {
        case Status::Todo: return "todo";
        case Status::InProgress: return "in_progress";
        case Status::Done: return "done";
    }
    return "todo";
}

std::optional<Status> status_from_string(std::string_view text) {
    if (text == "todo") return Status::Todo;
    if (text == "in_progress") return Status::InProgress;
    if (text == "done") return Status::Done;
    return std::nullopt;
}

void to_json(nlohmann::json& j, const Task& task) {
    j = nlohmann::json{
        {"id", task.id},
        {"title", task.title},
        {"description", task.description},
        {"status", to_string(task.status)},
        {"created_at", task.created_at},
    };
}

TaskPatch parse_patch(const nlohmann::json& body) {
    if (!body.is_object()) {
        throw std::invalid_argument("body must be a JSON object");
    }
    TaskPatch patch;
    if (body.contains("title")) {
        if (!body["title"].is_string()) throw std::invalid_argument("title must be a string");
        patch.title = body["title"].get<std::string>();
    }
    if (body.contains("description")) {
        if (!body["description"].is_string()) {
            throw std::invalid_argument("description must be a string");
        }
        patch.description = body["description"].get<std::string>();
    }
    if (body.contains("status")) {
        if (!body["status"].is_string()) throw std::invalid_argument("status must be a string");
        auto status = status_from_string(body["status"].get<std::string>());
        if (!status) {
            throw std::invalid_argument("status must be one of: todo, in_progress, done");
        }
        patch.status = *status;
    }
    return patch;
}

std::string now_iso8601() {
    const auto now = std::chrono::system_clock::now();
    const std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
#if defined(_WIN32)
    gmtime_s(&tm, &t);
#else
    gmtime_r(&t, &tm);
#endif
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &tm);
    return buf;
}

}  // namespace taskboard
