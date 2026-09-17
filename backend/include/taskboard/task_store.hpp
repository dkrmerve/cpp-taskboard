#pragma once

#include <cstddef>
#include <cstdint>
#include <map>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

#include "taskboard/task.hpp"

namespace taskboard {

struct Stats {
    std::size_t total{};
    std::size_t todo{};
    std::size_t in_progress{};
    std::size_t done{};
};

/// Thread-safe in-memory task repository.
class TaskStore {
public:
    /// Creates a task. Title must be non-empty after trimming.
    /// Throws std::invalid_argument otherwise.
    Task create(const TaskPatch& input);

    std::optional<Task> get(std::uint64_t id) const;

    /// Lists tasks ordered by id, optionally filtered by status.
    std::vector<Task> list(std::optional<Status> filter = std::nullopt) const;

    /// Applies a partial update. Returns nullopt if the task does not exist.
    /// Throws std::invalid_argument if the resulting title would be empty.
    std::optional<Task> update(std::uint64_t id, const TaskPatch& patch);

    bool remove(std::uint64_t id);

    Stats stats() const;
    std::size_t size() const;
    void clear();

private:
    mutable std::mutex mutex_;
    std::map<std::uint64_t, Task> tasks_;
    std::uint64_t next_id_{1};
};

std::string trim(const std::string& text);

}  // namespace taskboard
