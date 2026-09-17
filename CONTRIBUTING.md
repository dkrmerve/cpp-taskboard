# Contributing

## Branch model

| Branch        | Purpose                                        | Merges into |
|---------------|------------------------------------------------|-------------|
| `main`        | Production-ready code. Every push publishes images via the CD workflow. | – |
| `develop`     | Integration branch. All features land here first. | `main` (release PR) |
| `feature/*`   | New functionality, e.g. `feature/task-priority` | `develop` |
| `fix/*`       | Non-urgent bug fixes                           | `develop` |
| `hotfix/*`    | Urgent production fixes                        | `main` **and** `develop` |

### Typical flow

```bash
git checkout develop
git pull
git checkout -b feature/my-change
# ... work, commit ...
git push -u origin feature/my-change
gh pr create --base develop
```

When `develop` is ready to ship, open a PR from `develop` to `main` and tag the
merge commit (`git tag v0.2.0 && git push --tags`). The CD workflow publishes
`ghcr.io/<owner>/cpp-taskboard-backend:<version>` and the matching frontend image.

## Commit messages

Use [Conventional Commits](https://www.conventionalcommits.org/):

```
feat(api): add priority field to tasks
fix(store): trim title before validation
ci: cache FetchContent dependencies
```

## Before opening a PR

```bash
cmake -S backend -B backend/build -DCMAKE_BUILD_TYPE=Debug
cmake --build backend/build
ctest --test-dir backend/build --output-on-failure
```

Or, without a local toolchain:

```bash
docker compose build   # tests run inside the backend image build
```

Format C++ with `clang-format -i` (config in `.clang-format`).
