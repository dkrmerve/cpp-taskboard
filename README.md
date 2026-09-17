# cpp-taskboard

[![CI](https://github.com/dkrmerve/cpp-taskboard/actions/workflows/ci.yml/badge.svg)](https://github.com/dkrmerve/cpp-taskboard/actions/workflows/ci.yml)
[![CD](https://github.com/dkrmerve/cpp-taskboard/actions/workflows/cd.yml/badge.svg)](https://github.com/dkrmerve/cpp-taskboard/actions/workflows/cd.yml)

A small Kanban-style task board: a **C++20 REST backend** (cpp-httplib + nlohmann/json)
and a **vanilla JS frontend** served by nginx, wired together with Docker Compose and
GitHub Actions.

```
┌────────────┐   /api/*    ┌──────────────────┐
│  frontend  │ ──────────▶ │  backend (C++20) │
│  nginx:80  │             │  httplib :8080   │
└────────────┘             └──────────────────┘
   :3000 on host
```

## Quick start (Docker)

```bash
docker compose up --build
```

Open <http://localhost:3000>. The API is proxied at `http://localhost:3000/api`
and also exposed directly on `http://localhost:8080/api`.

The backend image runs the full test suite during `docker build`, so a red test
means no image.

## Local build (CMake ≥ 3.20, C++20 compiler)

```bash
cmake -S backend -B backend/build -DCMAKE_BUILD_TYPE=Release
cmake --build backend/build --parallel
ctest --test-dir backend/build --output-on-failure
PORT=8080 STATIC_DIR=frontend ./backend/build/taskboard_server
```

With `STATIC_DIR` set, the backend serves the frontend itself, so nginx is
optional for local development.

## API

| Method | Path                         | Description                              |
|--------|------------------------------|------------------------------------------|
| GET    | `/api/health`                | Liveness probe + version                 |
| GET    | `/api/stats`                 | Counts per status                        |
| GET    | `/api/tasks[?status=done]`   | List tasks, optional status filter       |
| POST   | `/api/tasks`                 | Create `{title, description?, status?}`  |
| GET    | `/api/tasks/{id}`            | Fetch one task                           |
| PUT    | `/api/tasks/{id}`            | Partial update of title/description/status |
| DELETE | `/api/tasks/{id}`            | Remove task (204)                        |
| DELETE | `/api/tasks?status=done`     | Bulk remove by status, returns `{removed}` |

Statuses: `todo`, `in_progress`, `done`. Errors return `{"error": "..."}` with
400 or 404.

```bash
curl -X POST localhost:8080/api/tasks -H 'Content-Type: application/json' \
     -d '{"title":"Write README"}'
```

## Project layout

```
backend/
  include/taskboard/   public headers (task, task_store, api)
  src/                 implementation + main.cpp
  tests/               GoogleTest unit + HTTP API tests
  Dockerfile           multi-stage build, tests run in build stage
frontend/
  index.html, app.js, styles.css, nginx.conf, Dockerfile
.github/workflows/
  ci.yml               gcc + clang build/test, frontend check, compose smoke test
  cd.yml               publish images to GHCR on main / v* tags
docker-compose.yml
```

## Tests

* `task_test.cpp` – status parsing, JSON serialisation, patch validation
* `task_store_test.cpp` – CRUD, filtering, stats, thread-safety
* `api_test.cpp` – spins up the real HTTP server on a random port and exercises every route

## CI / CD

* **CI** runs on every push and PR to `main`/`develop`: builds with gcc and clang,
  runs `ctest`, checks the frontend JS, then builds the compose stack and hits the
  API through nginx.
* **CD** runs on pushes to `main` and `v*` tags and publishes
  `ghcr.io/dkrmerve/cpp-taskboard-backend` and `ghcr.io/dkrmerve/cpp-taskboard-frontend`.

## Branching

`main` (production) ← `develop` (integration) ← `feature/*`, `fix/*`.
See [CONTRIBUTING.md](CONTRIBUTING.md).

## License

MIT
