#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include <fstream>
#include "cache.hpp"

using namespace testing;

class FastCacheTest : public Test {
protected:
    void TearDown() override {
        std::remove("test_save_load.json");
        std::remove("test_bad.json");
        std::remove("test_fast.json");
    }
};

TEST_F(FastCacheTest, PutGetAndContains) {
    PersistentTimedLRUCache<std::string, std::string> cache(3, std::chrono::hours(24), "test_save_load.json");
    cache.Put("k1", "v1");
    cache.Put("k2", "v2");
    cache.Put("k3", "v3");
    cache.Put("k4", "v4"); 
    
    EXPECT_FALSE(cache.Contains("k1"));
    EXPECT_FALSE(cache.Get("k1").has_value());
    
    EXPECT_TRUE(cache.Contains("k2"));
    EXPECT_EQ(cache.Get("k2").value(), "v2");
    EXPECT_EQ(cache.GetAllKeys().size(), 3);
}

TEST_F(FastCacheTest, ExpiryBehavior) {
    PersistentTimedLRUCache<std::string, std::string> cache(3, std::chrono::milliseconds(1), "test_fast.json");
    cache.Put("k1", "v1");
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    EXPECT_FALSE(cache.Contains("k1"));
    EXPECT_FALSE(cache.Get("k1").has_value());
}

TEST_F(FastCacheTest, FileLoadSave) {
    {
        PersistentTimedLRUCache<std::string, std::string> cache1(3, std::chrono::hours(24), "test_save_load.json");
        cache1.Put("k1", "v1");
    }
    {
        PersistentTimedLRUCache<std::string, std::string> cache2(3, std::chrono::hours(24), "test_save_load.json");
        EXPECT_EQ(cache2.Size(), 1);
        EXPECT_EQ(cache2.Get("k1").value_or(""), "v1");
    }
}

TEST_F(FastCacheTest, BadFileHandling) {
    {
        std::ofstream out("test_bad.json");
        out << "invalid_json_here";
    }
    PersistentTimedLRUCache<std::string, std::string> cache(3, std::chrono::hours(24), "test_bad.json");
    EXPECT_EQ(cache.Size(), 0);
}

TEST_F(FastCacheTest, ClearAndTimes) {
    PersistentTimedLRUCache<std::string, std::string> cache(3, std::chrono::hours(24), "test_save_load.json");
    cache.Put("k1", "v1");
    EXPECT_TRUE(cache.GetEntryTimes("k1").has_value());
    cache.Clear();
    EXPECT_FALSE(cache.GetEntryTimes("k1").has_value());
}
