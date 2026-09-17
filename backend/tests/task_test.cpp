#include <gtest/gtest.h>

#include <nlohmann/json.hpp>

#include "taskboard/task.hpp"

using namespace taskboard;
using nlohmann::json;

TEST(Status, RoundTripsThroughString) {
    for (Status s : {Status::Todo, Status::InProgress, Status::Done}) {
        EXPECT_EQ(status_from_string(to_string(s)), s);
    }
}

TEST(Status, RejectsUnknownStrings) {
    EXPECT_FALSE(status_from_string("").has_value());
    EXPECT_FALSE(status_from_string("DONE").has_value());
    EXPECT_FALSE(status_from_string("in-progress").has_value());
}

TEST(TaskJson, SerializesAllFields) {
    Task t{42, "Write docs", "README first", Status::InProgress, "2026-09-17T10:00:00Z"};
    const json j = t;
    EXPECT_EQ(j["id"], 42);
    EXPECT_EQ(j["title"], "Write docs");
    EXPECT_EQ(j["description"], "README first");
    EXPECT_EQ(j["status"], "in_progress");
    EXPECT_EQ(j["created_at"], "2026-09-17T10:00:00Z");
}

TEST(ParsePatch, ReadsOptionalFields) {
    const auto patch = parse_patch(json{{"title", "A"}, {"status", "done"}});
    ASSERT_TRUE(patch.title.has_value());
    EXPECT_EQ(*patch.title, "A");
    EXPECT_FALSE(patch.description.has_value());
    ASSERT_TRUE(patch.status.has_value());
    EXPECT_EQ(*patch.status, Status::Done);
}

TEST(ParsePatch, RejectsWrongTypes) {
    EXPECT_THROW(parse_patch(json::array()), std::invalid_argument);
    EXPECT_THROW(parse_patch(json{{"title", 1}}), std::invalid_argument);
    EXPECT_THROW(parse_patch(json{{"description", true}}), std::invalid_argument);
    EXPECT_THROW(parse_patch(json{{"status", "bogus"}}), std::invalid_argument);
}

TEST(NowIso8601, HasExpectedShape) {
    const std::string ts = now_iso8601();
    ASSERT_EQ(ts.size(), 20u);
    EXPECT_EQ(ts[4], '-');
    EXPECT_EQ(ts[10], 'T');
    EXPECT_EQ(ts.back(), 'Z');
}
