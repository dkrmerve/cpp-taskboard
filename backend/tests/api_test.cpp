#include <gtest/gtest.h>

#include <chrono>
#include <memory>
#include <thread>

#include <httplib.h>
#include <nlohmann/json.hpp>

#include "taskboard/api.hpp"
#include "taskboard/task_store.hpp"

using nlohmann::json;

class ApiTest : public ::testing::Test {
protected:
    void SetUp() override {
        taskboard::register_routes(server_, store_);
        port_ = server_.bind_to_any_port("127.0.0.1");
        ASSERT_GT(port_, 0);
        thread_ = std::thread([this] { server_.listen_after_bind(); });
        server_.wait_until_ready();
        client_ = std::make_unique<httplib::Client>("127.0.0.1", port_);
    }

    void TearDown() override {
        server_.stop();
        if (thread_.joinable()) thread_.join();
    }

    json post_task(const json& body) {
        auto res = client_->Post("/api/tasks", body.dump(), "application/json");
        EXPECT_TRUE(res);
        EXPECT_EQ(res->status, 201);
        return json::parse(res->body);
    }

    taskboard::TaskStore store_;
    httplib::Server server_;
    std::thread thread_;
    int port_{};
    std::unique_ptr<httplib::Client> client_;
};

TEST_F(ApiTest, HealthReportsOkAndVersion) {
    auto res = client_->Get("/api/health");
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 200);
    EXPECT_EQ(res->get_header_value("Content-Type"), "application/json");
    const json body = json::parse(res->body);
    EXPECT_EQ(body["status"], "ok");
    EXPECT_EQ(body["version"], TASKBOARD_VERSION);
}

TEST_F(ApiTest, CorsHeadersArePresent) {
    auto res = client_->Options("/api/tasks");
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 204);
    EXPECT_EQ(res->get_header_value("Access-Control-Allow-Origin"), "*");
}

TEST_F(ApiTest, ListStartsEmpty) {
    auto res = client_->Get("/api/tasks");
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 200);
    EXPECT_EQ(json::parse(res->body), json::array());
}

TEST_F(ApiTest, CreateReturns201WithTask) {
    const json created = post_task({{"title", "Ship it"}, {"description", "today"}});
    EXPECT_EQ(created["id"], 1);
    EXPECT_EQ(created["title"], "Ship it");
    EXPECT_EQ(created["description"], "today");
    EXPECT_EQ(created["status"], "todo");
    EXPECT_EQ(store_.size(), 1u);
}

TEST_F(ApiTest, CreateValidatesInput) {
    auto missing = client_->Post("/api/tasks", "{}", "application/json");
    ASSERT_TRUE(missing);
    EXPECT_EQ(missing->status, 400);
    EXPECT_EQ(json::parse(missing->body)["error"], "title is required");

    auto bad_json = client_->Post("/api/tasks", "{not json", "application/json");
    ASSERT_TRUE(bad_json);
    EXPECT_EQ(bad_json->status, 400);

    auto empty = client_->Post("/api/tasks", "", "application/json");
    ASSERT_TRUE(empty);
    EXPECT_EQ(empty->status, 400);

    auto bad_status = client_->Post("/api/tasks", R"({"title":"x","status":"nope"})",
                                    "application/json");
    ASSERT_TRUE(bad_status);
    EXPECT_EQ(bad_status->status, 400);
}

TEST_F(ApiTest, GetByIdAnd404) {
    post_task({{"title", "one"}});
    auto ok = client_->Get("/api/tasks/1");
    ASSERT_TRUE(ok);
    EXPECT_EQ(ok->status, 200);
    EXPECT_EQ(json::parse(ok->body)["title"], "one");

    auto missing = client_->Get("/api/tasks/999");
    ASSERT_TRUE(missing);
    EXPECT_EQ(missing->status, 404);

    auto not_numeric = client_->Get("/api/tasks/abc");
    ASSERT_TRUE(not_numeric);
    EXPECT_EQ(not_numeric->status, 404);  // route does not match
}

TEST_F(ApiTest, UpdateChangesStatus) {
    post_task({{"title", "move me"}});
    auto res = client_->Put("/api/tasks/1", R"({"status":"done"})", "application/json");
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 200);
    EXPECT_EQ(json::parse(res->body)["status"], "done");
    EXPECT_EQ(store_.get(1)->status, taskboard::Status::Done);

    auto missing = client_->Put("/api/tasks/5", R"({"status":"done"})", "application/json");
    ASSERT_TRUE(missing);
    EXPECT_EQ(missing->status, 404);

    auto invalid = client_->Put("/api/tasks/1", R"({"title":""})", "application/json");
    ASSERT_TRUE(invalid);
    EXPECT_EQ(invalid->status, 400);
}

TEST_F(ApiTest, DeleteRemovesTask) {
    post_task({{"title", "bye"}});
    auto res = client_->Delete("/api/tasks/1");
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 204);
    EXPECT_EQ(store_.size(), 0u);

    auto again = client_->Delete("/api/tasks/1");
    ASSERT_TRUE(again);
    EXPECT_EQ(again->status, 404);
}

TEST_F(ApiTest, ListFilterAndStats) {
    post_task({{"title", "a"}});
    post_task({{"title", "b"}, {"status", "done"}});
    post_task({{"title", "c"}, {"status", "in_progress"}});

    auto done = client_->Get("/api/tasks?status=done");
    ASSERT_TRUE(done);
    const json list = json::parse(done->body);
    ASSERT_EQ(list.size(), 1u);
    EXPECT_EQ(list[0]["title"], "b");

    auto bad_filter = client_->Get("/api/tasks?status=weird");
    ASSERT_TRUE(bad_filter);
    EXPECT_EQ(bad_filter->status, 400);

    auto stats = client_->Get("/api/stats");
    ASSERT_TRUE(stats);
    const json s = json::parse(stats->body);
    EXPECT_EQ(s["total"], 3);
    EXPECT_EQ(s["todo"], 1);
    EXPECT_EQ(s["in_progress"], 1);
    EXPECT_EQ(s["done"], 1);
}
