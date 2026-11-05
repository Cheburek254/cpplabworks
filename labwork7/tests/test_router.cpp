#include <gtest/gtest.h>
#include <cstdlib>
#include "router.hpp"

using namespace testing;

#ifdef _WIN32
int setenv(const char* name, const char* value, int) {
    return _putenv_s(name, value);
}
#endif

class FastRouterTest : public Test {
protected:
    static void SetUpTestSuite() {
        setenv("YANDEX_API_KEY", "test_key", 1);
    }
};

TEST_F(FastRouterTest, CityCodeMapping) {
    const auto& codes = Router::GetCityCodeMap();
    EXPECT_EQ(codes.at("Penza"), "c49");
    EXPECT_EQ(codes.at("Moscow"), "c213");
    EXPECT_EQ(codes.at("Saint Petersburg"), "c2");
}

TEST_F(FastRouterTest, CreateRouterAndMethods) {
    Router router("test_key", "Penza", 3);
    EXPECT_EQ(router.GetHomeCity(), "Penza");
    EXPECT_TRUE(router.FindRoutesToHome("2025-01-01", 1, 1).empty());
    EXPECT_TRUE(router.FindRoutesFromHome("2025-01-01", 1, 1).empty());
}

TEST_F(FastRouterTest, InvalidCity) {
    EXPECT_THROW({
        Router router("test_key", "UnknownCity", 3);
    }, std::runtime_error);
}
