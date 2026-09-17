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

## Running notes

### Ports and environment

| Variable     | Default   | Used by  | Meaning                                              |
|--------------|-----------|----------|------------------------------------------------------|
| `PORT`       | `8080`    | backend  | HTTP listen port                                     |
| `HOST`       | `0.0.0.0` | backend  | Bind address (`127.0.0.1` to keep it local-only)     |
| `STATIC_DIR` | *(unset)* | backend  | If set, serves that folder at `/` (frontend without nginx) |

| Host port | Service  | Notes                                   |
|-----------|----------|-----------------------------------------|
| `3000`    | frontend | nginx; `/api/*` is proxied to backend    |
| `8080`    | backend  | Direct API access, handy for curl/Postman |

### Useful commands

```bash
docker compose up --build -d      # start in background
docker compose logs -f backend    # request log: "POST /api/tasks -> 201"
docker compose ps                 # both services should say (healthy)
docker compose down               # stop; add -v to drop volumes (none today)
```

### Troubleshooting

* **Frontend loads but shows a red dot** – backend is down or not yet healthy.
  `docker compose logs backend` and `curl localhost:8080/api/health`.
* **Port already in use** – change the host side of the mapping in
  `docker-compose.yml` (`"3001:80"`), the container side stays as is.
* **Container reports unhealthy on Alpine/nginx** – healthchecks must use
  `127.0.0.1`, not `localhost` (resolves to IPv6 first, nginx listens on IPv4).
* **`docker build` slow the first time** – FetchContent downloads cpp-httplib,
  nlohmann/json and GoogleTest; later builds hit the layer cache.
* **Data disappears on restart** – expected, the store is in-memory (see below).

## Edge cases handled

| Case                                        | Behaviour                                   | Covered by test |
|---------------------------------------------|---------------------------------------------|-----------------|
| Empty / whitespace-only title               | 400 `title is required`, nothing stored     | `CreateTrimsTitleAndRejectsEmpty`, `CreateValidatesInput` |
| Title padded with spaces                    | Trimmed before storing                      | `CreateTrimsTitleAndRejectsEmpty` |
| Update that would blank the title           | 400, original task untouched                | `UpdateRejectsEmptyTitleAndKeepsOriginal` |
| Empty body / malformed JSON / wrong field type | 400 with a specific error message        | `CreateValidatesInput`, `ParsePatch.RejectsWrongTypes` |
| Unknown status value (`"DONE"`, `"in-progress"`) | 400, only `todo`, `in_progress`, `done` accepted | `Status.RejectsUnknownStrings` |
| Unknown `?status=` filter                   | 400 instead of silently returning nothing   | `ListFilterAndStats` |
| Non-existent id on GET/PUT/DELETE           | 404                                         | `GetByIdAnd404`, `UpdateChangesStatus`, `DeleteRemovesTask` |
| Non-numeric id (`/api/tasks/abc`)           | Route does not match → 404                  | `GetByIdAnd404` |
| Deleting the same task twice                | Second call returns 404                     | `DeleteRemovesTask` |
| Concurrent creates from many threads        | Mutex-guarded store, ids stay unique and ordered | `ConcurrentCreatesProduceUniqueIds` |
| Partial update (`{"status": "done"}` only)  | Other fields keep their values              | `UpdateAppliesPartialPatch` |
| Browser on a different origin               | CORS headers + `OPTIONS` preflight → 204    | `CorsHeadersArePresent` |

Known limits, by design for this version: ids are 64-bit and never reused;
there is no maximum title length on the API side (the UI caps at 120/500 chars);
`created_at` is second-precision UTC.

## Before going to production

The stack is a complete, tested demo. Before exposing it to real users:

- [ ] **Persistence** – the store is in-memory, so a restart wipes every task. Swap
      `TaskStore` for SQLite/PostgreSQL behind the same interface (the API and
      tests are already decoupled from the storage).
- [ ] **Authentication / authorisation** – every endpoint is public. Put the API
      behind a reverse proxy with auth, or add token checks in `register_routes`.
- [ ] **Lock down CORS** – `Access-Control-Allow-Origin: *` is for development.
      Set it to the real frontend origin.
- [ ] **TLS** – terminate HTTPS at nginx or a load balancer; the backend speaks
      plain HTTP.
- [ ] **Request limits** – set `server.set_payload_max_length(...)`, read/write
      timeouts and a title length cap to avoid abuse.
- [ ] **Graceful shutdown** – handle `SIGTERM` and call `server.stop()` so
      rolling deploys do not cut requests mid-flight.
- [ ] **Observability** – switch the stdout logger to structured JSON logs, add a
      `/metrics` endpoint (Prometheus) and a request-id header.
- [ ] **Resource limits** – add `deploy.resources.limits` (or Kubernetes
      requests/limits) and a non-`unless-stopped` restart policy suited to the
      orchestrator.
- [ ] **Image hygiene** – pin base images by digest, run Trivy/Grype in CI, and
      tag releases (`v1.0.0`) so CD publishes immutable versions.
- [ ] **Secrets / config** – keep environment in a secrets manager, never in the
      compose file committed to git.
- [ ] **Backups & migrations** – once persistence exists, script schema
      migrations and automated backups before the first real deploy.

## Branching

`main` (production) ← `develop` (integration) ← `feature/*`, `fix/*`.
See [CONTRIBUTING.md](CONTRIBUTING.md).

## License

MIT
