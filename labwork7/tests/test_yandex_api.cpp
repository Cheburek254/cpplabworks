#include <gtest/gtest.h>
#include <nlohmann/json.hpp>
#include "yandex_api.hpp"
#include "mock_yandex_api.hpp"

using namespace testing;
using json = nlohmann::json;

static const char* kExhaustiveJson = R"({
    "segments": [
        {
            "departure_from": {"title": "DepCity", "code": "d1", "popular_title": "dp"},
            "arrival_to": {"title": "ArrCity", "code": "a1", "popular_title": "ap"},
            "departure": "2025-01-01T10:00:00",
            "arrival": "2025-01-01T12:00:00",
            "duration": 7200,
            "has_transfers": false,
            "departure_terminal": "A",
            "arrival_terminal": "B",
            "thread": {
                "uid": "123",
                "title": "ThreadTitle",
                "number": "T1",
                "short_title": "ShortT",
                "transport_type": "train",
                "vehicle": "train_model",
                "carrier": {
                    "code": 999,
                    "title": "Carrier",
                    "url": "http",
                    "phone": "123",
                    "contacts": "cnt",
                    "logo": "logo.png"
                }
            },
            "tickets_info": {
                "et_marker": true,
                "places": [
                    {
                        "name": "Seat",
                        "currency": "RUB",
                        "price": { "cents": 50, "whole": 100, "currency": "RUB" }
                    },
                    null
                ]
            }
        },
        { "from": "StringCity1", "to": "StringCity2", "duration": 3600 },
        { "has_transfers": true, "details": [ { "is_transfer": true }, { "from": {"title": "T1"}, "to": {"title": "T2"}, "duration": 1800 } ] },
        { "from": {"code": "1", "popular_title": "Pop1"}, "to": {"code": "2", "popular_title": "Pop2"}, "duration": 1000 }
    ]
})";

static const char* kBadJsonDuration = R"({
    "segments": [
        { "from": "A", "to": "B", "duration": -10 },
        { "from": "A", "to": "B", "duration": 999999999 }
    ]
})";

class FastYandexAPITest : public Test {
protected:
    void SetUp() override {
        api_ = std::make_unique<TestableYandexAPI>("test_key", 3);
    }
    std::unique_ptr<TestableYandexAPI> api_;
};

TEST_F(FastYandexAPITest, ParseExhaustiveSegments) {
    auto routes = api_->ParseRoutes(json::parse(kExhaustiveJson));
    ASSERT_FALSE(routes.empty());
    EXPECT_EQ(routes[0].segments[0].from.title, "DepCity");
    EXPECT_EQ(routes[0].segments[0].departure_terminal, "A");
    EXPECT_EQ(routes[0].segments[0].thread.carrier.title, "Carrier");
    EXPECT_TRUE(routes[0].segments[0].has_tickets_info);
    EXPECT_EQ(routes[1].segments[0].from.title, "StringCity1");
    EXPECT_EQ(routes[2].segments[0].from.title, "T1");
    EXPECT_EQ(routes[3].segments[0].from.title, "Pop1");
}

TEST_F(FastYandexAPITest, EdgeCasesAndErrors) {
    EXPECT_TRUE(api_->ParseRoutes(json::parse(R"({"not_segments":[]})")).empty());
    EXPECT_TRUE(api_->ParseRoutes(json::parse(R"({"segments": [{"from": null}]})")).empty());
    EXPECT_TRUE(api_->ParseRoutes(json::parse(kBadJsonDuration)).empty());
}

TEST_F(FastYandexAPITest, CacheAndInfo) {
    testing::internal::CaptureStdout();
    api_->ShowCacheInfo();
    EXPECT_NE(testing::internal::GetCapturedStdout().find("Cache Information"), std::string::npos);

    testing::internal::CaptureStdout();
    api_->ClearCache();
    EXPECT_NE(testing::internal::GetCapturedStdout().find("Cache cleared"), std::string::npos);
}

TEST_F(FastYandexAPITest, FilterAndCacheKey) {
    EXPECT_EQ(api_->MakeCacheKey("a", "b", "c"), "a|b|c");

    std::vector<Route> test_routes;
    for (int i = 1; i <= 5; ++i) {
        Route r;
        r.segments.resize(i);
        r.total_duration = 1000 - i;
        test_routes.push_back(r);
    }
    EXPECT_EQ(api_->FilterRoutes(test_routes, 2, 10).size(), 3);
    EXPECT_EQ(api_->FilterRoutes(test_routes, 5, 2).size(), 2);
}

TEST_F(FastYandexAPITest, NetworkMocksAndFallback) {
    auto routes = api_->FindRoutes("c2", "c49", "2025-01-01", 1, 1);
    EXPECT_TRUE(routes.empty());
}
