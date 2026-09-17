#include "taskboard/task_store.hpp"

#include <stdexcept>

namespace taskboard {

std::string trim(const std::string& text) {
    const auto first = text.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    const auto last = text.find_last_not_of(" \t\r\n");
    return text.substr(first, last - first + 1);
}

Task TaskStore::create(const TaskPatch& input) {
    const std::string title = trim(input.title.value_or(""));
    if (title.empty()) {
        throw std::invalid_argument("title is required");
    }
    std::lock_guard lock(mutex_);
    Task task;
    task.id = next_id_++;
    task.title = title;
    task.description = input.description.value_or("");
    task.status = input.status.value_or(Status::Todo);
    task.created_at = now_iso8601();
    tasks_.emplace(task.id, task);
    return task;
}

std::optional<Task> TaskStore::get(std::uint64_t id) const {
    std::lock_guard lock(mutex_);
    const auto it = tasks_.find(id);
    if (it == tasks_.end()) return std::nullopt;
    return it->second;
}

std::vector<Task> TaskStore::list(std::optional<Status> filter) const {
    std::lock_guard lock(mutex_);
    std::vector<Task> out;
    out.reserve(tasks_.size());
    for (const auto& [id, task] : tasks_) {
        (void)id;
        if (!filter || task.status == *filter) out.push_back(task);
    }
    return out;
}

std::optional<Task> TaskStore::update(std::uint64_t id, const TaskPatch& patch) {
    std::lock_guard lock(mutex_);
    const auto it = tasks_.find(id);
    if (it == tasks_.end()) return std::nullopt;

    Task updated = it->second;
    if (patch.title) {
        const std::string title = trim(*patch.title);
        if (title.empty()) throw std::invalid_argument("title cannot be empty");
        updated.title = title;
    }
    if (patch.description) updated.description = *patch.description;
    if (patch.status) updated.status = *patch.status;

    it->second = updated;
    return updated;
}

bool TaskStore::remove(std::uint64_t id) {
    std::lock_guard lock(mutex_);
    return tasks_.erase(id) > 0;
}

Stats TaskStore::stats() const {
    std::lock_guard lock(mutex_);
    Stats s;
    s.total = tasks_.size();
    for (const auto& [id, task] : tasks_) {
        (void)id;
        switch (task.status) {
            case Status::Todo: ++s.todo; break;
            case Status::InProgress: ++s.in_progress; break;
            case Status::Done: ++s.done; break;
        }
    }
    return s;
}

std::size_t TaskStore::size() const {
    std::lock_guard lock(mutex_);
    return tasks_.size();
}

void TaskStore::clear() {
    std::lock_guard lock(mutex_);
    tasks_.clear();
    next_id_ = 1;
}

}  // namespace taskboard
