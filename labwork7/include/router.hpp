#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include "yandex_api.hpp"

class Router {
private:
    YandexAPI api_;
    std::string home_city_;
    std::string home_code_;
    std::string spb_code_ = "c2"; 
    std::unordered_map<std::string, std::string> city_names_;

    std::optional<std::string> GetCityCode(const std::string& city_name);

public:
    Router(const std::string& api_key, const std::string& home_city, int default_routes = 3);
    
    std::vector<Route> FindRoutesToHome(const std::string& date, int max_routes = -1, int max_transfers = 1);
    std::vector<Route> FindRoutesFromHome(const std::string& date, int max_routes = -1, int max_transfers = 1);

    YandexAPI& GetApi() { return api_; }
    const std::string& GetHomeCity() const { return home_city_; }
    
    static const std::unordered_map<std::string, std::string>& GetCityCodeMap();
    
    void ShowCityCache() const;
};