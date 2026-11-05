#pragma once

#include <gmock/gmock.h>
#include <string>
#include <vector>
#include "yandex_api.hpp"
#include "models.hpp"

class MockYandexAPI {
public:
    MOCK_METHOD(std::vector<Route>, FindRoutes, 
                (const std::string& from, 
                 const std::string& to, 
                 const std::string& date,
                 int max_routes,
                 int max_transfers),
                (const));
    
    MOCK_METHOD(void, ShowCacheInfo, (), (const));
    MOCK_METHOD(void, ClearCache, ());
    MOCK_METHOD(size_t, CacheSize, (), (const));
};


class TestableYandexAPI : public YandexAPI {
public:
    using YandexAPI::YandexAPI;
    
    using YandexAPI::ParseRoutes;
    using YandexAPI::ParseSegment;
    using YandexAPI::FilterRoutes;
    using YandexAPI::MakeCacheKey;
};