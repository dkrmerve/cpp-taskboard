#pragma once

#include <httplib.h>

#include "taskboard/task_store.hpp"

namespace taskboard {

/// Registers all REST routes under /api on the given server.
void register_routes(httplib::Server& server, TaskStore& store);

}  // namespace taskboard
