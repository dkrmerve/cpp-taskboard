#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

#include <nlohmann/json.hpp>

namespace taskboard {

enum class Status { Todo, InProgress, Done };

std::string to_string(Status status);
std::optional<Status> status_from_string(std::string_view text);

struct Task {
    std::uint64_t id{};
    std::string title;
    std::string description;
    Status status{Status::Todo};
    std::string created_at;  // ISO-8601 UTC
};

/// Fields a client may send when creating or updating a task.
struct TaskPatch {
    std::optional<std::string> title;
    std::optional<std::string> description;
    std::optional<Status> status;
};

void to_json(nlohmann::json& j, const Task& task);

/// Parses a JSON body into a patch. Throws std::invalid_argument on bad input.
TaskPatch parse_patch(const nlohmann::json& body);

/// Current time as ISO-8601 UTC ("2026-09-17T10:15:30Z").
std::string now_iso8601();

}  // namespace taskboard
