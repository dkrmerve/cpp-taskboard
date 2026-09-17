#include <gtest/gtest.h>

#include <stdexcept>
#include <thread>
#include <vector>

#include "taskboard/task_store.hpp"

using namespace taskboard;

namespace {
TaskPatch input(std::string title, std::string description = "",
                std::optional<Status> status = std::nullopt) {
    TaskPatch p;
    p.title = std::move(title);
    p.description = std::move(description);
    p.status = status;
    return p;
}
}  // namespace

TEST(Trim, StripsWhitespace) {
    EXPECT_EQ(trim("  hi  "), "hi");
    EXPECT_EQ(trim("\t\n"), "");
    EXPECT_EQ(trim("x"), "x");
}

TEST(TaskStore, CreateAssignsIncrementingIds) {
    TaskStore store;
    const Task a = store.create(input("first"));
    const Task b = store.create(input("second"));
    EXPECT_EQ(a.id, 1u);
    EXPECT_EQ(b.id, 2u);
    EXPECT_EQ(a.status, Status::Todo);
    EXPECT_FALSE(a.created_at.empty());
    EXPECT_EQ(store.size(), 2u);
}

TEST(TaskStore, CreateTrimsTitleAndRejectsEmpty) {
    TaskStore store;
    EXPECT_EQ(store.create(input("  padded  ")).title, "padded");
    EXPECT_THROW(store.create(input("   ")), std::invalid_argument);
    EXPECT_THROW(store.create(TaskPatch{}), std::invalid_argument);
    EXPECT_EQ(store.size(), 1u);
}

TEST(TaskStore, GetReturnsNulloptForMissing) {
    TaskStore store;
    EXPECT_FALSE(store.get(99).has_value());
    store.create(input("x"));
    ASSERT_TRUE(store.get(1).has_value());
    EXPECT_EQ(store.get(1)->title, "x");
}

TEST(TaskStore, ListFiltersByStatus) {
    TaskStore store;
    store.create(input("a", "", Status::Todo));
    store.create(input("b", "", Status::Done));
    store.create(input("c", "", Status::Done));

    EXPECT_EQ(store.list().size(), 3u);
    EXPECT_EQ(store.list(Status::Done).size(), 2u);
    EXPECT_EQ(store.list(Status::InProgress).size(), 0u);
    EXPECT_EQ(store.list(Status::Todo).front().title, "a");
}

TEST(TaskStore, UpdateAppliesPartialPatch) {
    TaskStore store;
    store.create(input("old", "desc"));

    TaskPatch patch;
    patch.status = Status::InProgress;
    const auto updated = store.update(1, patch);
    ASSERT_TRUE(updated.has_value());
    EXPECT_EQ(updated->title, "old");
    EXPECT_EQ(updated->description, "desc");
    EXPECT_EQ(updated->status, Status::InProgress);

    patch = TaskPatch{};
    patch.title = "new";
    EXPECT_EQ(store.update(1, patch)->title, "new");
}

TEST(TaskStore, UpdateRejectsEmptyTitleAndKeepsOriginal) {
    TaskStore store;
    store.create(input("keep"));
    TaskPatch patch;
    patch.title = "   ";
    EXPECT_THROW(store.update(1, patch), std::invalid_argument);
    EXPECT_EQ(store.get(1)->title, "keep");
}

TEST(TaskStore, UpdateMissingReturnsNullopt) {
    TaskStore store;
    TaskPatch patch;
    patch.title = "x";
    EXPECT_FALSE(store.update(7, patch).has_value());
}

TEST(TaskStore, RemoveReportsWhetherSomethingWasDeleted) {
    TaskStore store;
    store.create(input("x"));
    EXPECT_TRUE(store.remove(1));
    EXPECT_FALSE(store.remove(1));
    EXPECT_EQ(store.size(), 0u);
}

TEST(TaskStore, RemoveByStatusDeletesOnlyMatching) {
    TaskStore store;
    store.create(input("a", "", Status::Done));
    store.create(input("b", "", Status::Todo));
    store.create(input("c", "", Status::Done));

    EXPECT_EQ(store.remove_by_status(Status::Done), 2u);
    EXPECT_EQ(store.size(), 1u);
    EXPECT_EQ(store.list().front().title, "b");
    EXPECT_EQ(store.remove_by_status(Status::Done), 0u);
}

TEST(TaskStore, StatsCountPerStatus) {
    TaskStore store;
    store.create(input("a", "", Status::Todo));
    store.create(input("b", "", Status::InProgress));
    store.create(input("c", "", Status::Done));
    store.create(input("d", "", Status::Done));

    const Stats s = store.stats();
    EXPECT_EQ(s.total, 4u);
    EXPECT_EQ(s.todo, 1u);
    EXPECT_EQ(s.in_progress, 1u);
    EXPECT_EQ(s.done, 2u);
}

TEST(TaskStore, ClearResetsIds) {
    TaskStore store;
    store.create(input("a"));
    store.clear();
    EXPECT_EQ(store.size(), 0u);
    EXPECT_EQ(store.create(input("b")).id, 1u);
}

TEST(TaskStore, ConcurrentCreatesProduceUniqueIds) {
    TaskStore store;
    constexpr int kThreads = 8;
    constexpr int kPerThread = 200;

    std::vector<std::thread> workers;
    for (int t = 0; t < kThreads; ++t) {
        workers.emplace_back([&store] {
            for (int i = 0; i < kPerThread; ++i) store.create(input("t"));
        });
    }
    for (auto& w : workers) w.join();

    EXPECT_EQ(store.size(), static_cast<std::size_t>(kThreads * kPerThread));
    const auto all = store.list();
    for (std::size_t i = 0; i < all.size(); ++i) {
        EXPECT_EQ(all[i].id, i + 1);
    }
}
