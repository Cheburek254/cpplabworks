#include "router.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <optional>
#include <stdexcept>

Router::Router(const std::string& api_key, const std::string& home_city, int default_routes)
    : api_(api_key, default_routes), home_city_(home_city) {
    
    auto code = GetCityCode(home_city);
    if (code.has_value()) {
        home_code_ = code.value();
    } else {
        throw std::runtime_error("Unknown city code for: " + home_city);
    }
    
    for (const auto& [name, code] : GetCityCodeMap()) {
        city_names_[code] = name;
    }
}

std::optional<std::string> Router::GetCityCode(const std::string& city_name) {
    const auto& city_codes = GetCityCodeMap();

    auto it = city_codes.find(city_name);
    if (it != city_codes.end()) {
        return it->second;
    }
    return std::nullopt;
}

const std::unordered_map<std::string, std::string>& Router::GetCityCodeMap() {
    static const std::unordered_map<std::string, std::string> kCodes = {
        {"Moscow", "c213"},
        {"Saint Petersburg", "c2"},
        {"Penza", "c49"},
        {"Pskov", "c207"},
        {"Kazan", "c43"},
        {"Ekaterinburg", "c54"},
        {"Novosibirsk", "c4"},
        {"Samara", "c48"},
        {"Nizhny Novgorod", "c47"},
        {"Rostov-on-Don", "c39"},
        {"Ufa", "c60"},
        {"Volgograd", "c64"},
        {"Perm", "c52"},
        {"Voronezh", "c65"},
        {"Irkutsk", "c64"}
    };
    return kCodes;
}

std::vector<Route> Router::FindRoutesToHome(const std::string& date, int max_routes, int max_transfers) {
    return api_.FindRoutes("c2", home_code_, date, max_routes, max_transfers);
}

std::vector<Route> Router::FindRoutesFromHome(const std::string& date, int max_routes, int max_transfers) {
    return api_.FindRoutes(home_code_, "c2", date, max_routes, max_transfers);
}
