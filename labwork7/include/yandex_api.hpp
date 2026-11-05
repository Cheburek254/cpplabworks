#pragma once

#include <string>
#include <vector>
#include <optional>
#include <memory>
#include "cache.hpp"
#include "models.hpp"

class YandexAPI {
protected:  
    std::string api_key_;
    std::string base_url_ = "https://api.rasp.yandex.net/v3.0";
    PersistentTimedLRUCache<std::string, std::vector<Route>> cache_;
    int default_routes_count_;

    std::string MakeCacheKey(const std::string& from, const std::string& to, 
                               const std::string& date) const;
    
    std::vector<Route> ParseRoutes(const json& data) const;
    Segment ParseSegment(const json& item) const;
    std::vector<Route> FilterRoutes(const std::vector<Route>& routes, 
                                     int max_transfers, 
                                     int max_routes) const;

public:
    explicit YandexAPI(const std::string& api_key, int default_routes = 3);
    
    std::vector<Route> FindRoutes(const std::string& from, 
                                   const std::string& to, 
                                   const std::string& date,
                                   int max_routes = -1,
                                   int max_transfers = 1);
    
    void ShowCacheInfo() const;
    void ClearCache();
    size_t CacheSize() const { return cache_.Size(); }
};