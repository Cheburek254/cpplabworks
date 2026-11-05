#include <gtest/gtest.h>
#include <sstream>
#include <fstream>
#include "app.hpp"
#include "cache.hpp"

class AppTest : public ::testing::Test {
protected:
    void SetUp() override {
        original_cout_ = std::cout.rdbuf(cout_stream_.rdbuf());
        original_cerr_ = std::cerr.rdbuf(cerr_stream_.rdbuf());
        original_cin_ = std::cin.rdbuf(cin_stream_.rdbuf());

        PersistentTimedLRUCache<std::string, std::vector<Route>> cache(1000, std::chrono::hours(24), "routes_cache.json");
        std::vector<Route> test_routes;
        Route r;
        r.transfers_count = 1;
        r.total_duration = 3660;
        r.total_price = 150.5f;
        r.departure_time = "2025-01-01T10:00:00";
        r.arrival_time = "2025-01-01T11:01:00";
        
        Segment s;
        s.from.title = "A";
        s.to.title = "B";
        s.departure = "2025-01-01T10:00:00";
        s.arrival = "2025-01-01T10:30:00";
        s.thread.transport_type = "bus";
        s.thread.number = "123";
        s.thread.carrier.title = "Carrier1";
        s.departure_terminal = "T1";
        s.arrival_terminal = "T2";
        s.has_tickets_info = true;
        
        Place p;
        p.name = "Seat";
        p.price.cents = 50;
        p.price.whole = 100;
        p.price.currency = "RUB";
        p.currency = "RUB";
        s.tickets_info.places.push_back(p);
        
        r.segments.push_back(s);
        test_routes.push_back(r);
        
        cache.Put("c2|c49|2025-01-01", test_routes);
        cache.Put("c49|c2|2025-01-01", test_routes);
    }

    void TearDown() override {
        std::cout.rdbuf(original_cout_);
        std::cerr.rdbuf(original_cerr_);
        std::cin.rdbuf(original_cin_);
        std::remove("routes_cache.json");
    }

    std::ostringstream cout_stream_;
    std::ostringstream cerr_stream_;
    std::istringstream cin_stream_;
    std::streambuf* original_cout_;
    std::streambuf* original_cerr_;
    std::streambuf* original_cin_;
};

TEST_F(AppTest, RunCommands) {
    cin_stream_.str("help\ncache\nto_home 2025-01-01 -n 2 -t 1\nfrom_home 2025-01-01 -n 1 -t 0\nclear\nexit\n");
    App app("test_key", "Penza", 1);
    app.Run();
    std::string output = cout_stream_.str();
    EXPECT_TRUE(output.find("Carrier1") != std::string::npos);
    EXPECT_TRUE(output.find("A -> B") != std::string::npos);
}

TEST_F(AppTest, EdgeCases) {
    cin_stream_.str("to_home\ninvalid_cmd\nto_home 2025-01-02 -t 5\nexit\n");
    App app("test_key", "Penza", 1);
    app.Run();
    std::string output = cout_stream_.str();
    EXPECT_TRUE(output.find("Unknown command") != std::string::npos);
    EXPECT_TRUE(output.find("Transfers must be 0 or 1") != std::string::npos);
}