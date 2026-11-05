#include "yandex_api.hpp"
#include <iostream>
#include <algorithm>
#include <string>
#include <chrono>
#include <exception>
#include <vector>
#include <cstddef>
#include <cpr/cpr.h>
#include "models.hpp"
YandexAPI::YandexAPI(const std::string& api_key, int default_routes)
    : api_key_(api_key), 
      cache_(1000, std::chrono::hours(24), "routes_cache.json"),
      default_routes_count_(default_routes) {}

std::string YandexAPI::MakeCacheKey(const std::string& from, const std::string& to, 
                                      const std::string& date) const {
    return from + "|" + to + "|" + date;
}

Segment YandexAPI::ParseSegment(const json& item) const {
    Segment seg;
    
    try {
        
        if (item.contains("from") && !item["from"].is_null()) {
            const auto& from = item["from"];
            if (from.is_object()) {
                seg.from.code = from.value("code", "");
                seg.from.title = from.value("title", "");
                seg.from.popular_title = from.value("popular_title", "");
                seg.from.short_title = from.value("short_title", "");
                seg.from.transport_type = from.value("transport_type", "");
                seg.from.type = from.value("type", "");
                
                if (seg.from.title.empty()) {
                    seg.from.title = from.value("popular_title", "");
                }
            } else if (from.is_string()) {
                seg.from.title = from;
            }
        } else if (item.contains("departure_from") && !item["departure_from"].is_null()) {
            const auto& from = item["departure_from"];
            if (from.is_object()) {
                seg.from.code = from.value("code", "");
                seg.from.title = from.value("title", "");
                seg.from.popular_title = from.value("popular_title", "");
            }
        }
        
        
        if (item.contains("to") && !item["to"].is_null()) {
            const auto& to = item["to"];
            if (to.is_object()) {
                seg.to.code = to.value("code", "");
                seg.to.title = to.value("title", "");
                seg.to.popular_title = to.value("popular_title", "");
                seg.to.short_title = to.value("short_title", "");
                seg.to.transport_type = to.value("transport_type", "");
                seg.to.type = to.value("type", "");
                
                if (seg.to.title.empty()) {
                    seg.to.title = to.value("popular_title", "");
                }
            } else if (to.is_string()) {
                seg.to.title = to;
            }
        } else if (item.contains("arrival_to") && !item["arrival_to"].is_null()) {
            const auto& to = item["arrival_to"];
            if (to.is_object()) {
                seg.to.code = to.value("code", "");
                seg.to.title = to.value("title", "");
                seg.to.popular_title = to.value("popular_title", "");
            }
        }

        
        if (item.contains("departure") && !item["departure"].is_null()) {
            seg.departure = item["departure"];
        }
        
        if (item.contains("arrival") && !item["arrival"].is_null()) {
            seg.arrival = item["arrival"];
        }
        
        seg.duration = item.value("duration", 0);
        seg.has_transfers = item.value("has_transfers", false);

        
        if (item.contains("departure_terminal") && !item["departure_terminal"].is_null()) {
            if (item["departure_terminal"].is_string()) {
                seg.departure_terminal = item["departure_terminal"];
            }
        }
        if (item.contains("arrival_terminal") && !item["arrival_terminal"].is_null()) {
            if (item["arrival_terminal"].is_string()) {
                seg.arrival_terminal = item["arrival_terminal"];
            }
        }

        
        if (item.contains("thread") && !item["thread"].is_null() && item["thread"].is_object()) {
            const auto& thread = item["thread"];
            seg.thread.uid = thread.value("uid", "");
            seg.thread.title = thread.value("title", "");
            seg.thread.number = thread.value("number", "");
            seg.thread.short_title = thread.value("short_title", "");
            seg.thread.transport_type = thread.value("transport_type", "");
            seg.thread.vehicle = thread.value("vehicle", "");
            
            if (thread.contains("carrier") && !thread["carrier"].is_null() && thread["carrier"].is_object()) {
                const auto& carrier = thread["carrier"];
                seg.thread.carrier.code = carrier.value("code", 0);
                seg.thread.carrier.title = carrier.value("title", "");
                seg.thread.carrier.url = carrier.value("url", "");
                seg.thread.carrier.phone = carrier.value("phone", "");
                seg.thread.carrier.contacts = carrier.value("contacts", "");
                seg.thread.carrier.logo = carrier.value("logo", "");
            }
        }

        
        if (item.contains("tickets_info") && !item["tickets_info"].is_null() && item["tickets_info"].is_object()) {
            TicketsInfo tickets;
            const auto& tickets_json = item["tickets_info"];
            
            tickets.et_marker = tickets_json.value("et_marker", false);
            
            if (tickets_json.contains("places") && !tickets_json["places"].is_null() && tickets_json["places"].is_array()) {
                for (const auto& place_json : tickets_json["places"]) {
                    if (place_json.is_null()) continue;
                    
                    Place place;
                    place.name = place_json.value("name", "");
                    place.currency = place_json.value("currency", "RUB");
                    
                    if (place_json.contains("price") && !place_json["price"].is_null() && place_json["price"].is_object()) {
                        const auto& price_json = place_json["price"];
                        place.price.cents = price_json.value("cents", 0);
                        place.price.whole = price_json.value("whole", 0);
                        place.price.currency = place_json.value("currency", "RUB");
                    }
                    
                    tickets.places.push_back(place);
                }
            }
            
            seg.tickets_info = tickets;
            seg.has_tickets_info = true;
        } else {
            seg.has_tickets_info = false;
        }
        
    } catch (const std::exception& e) {
        
    }
    
    return seg;
}

std::vector<Route> YandexAPI::ParseRoutes(const json& data) const {
    std::vector<Route> routes;
    
    if (!data.contains("segments")) {
        return routes;
    }
    
    for (const auto& item : data["segments"]) {
        try {
            Route route;

            if (item.contains("has_transfers") && item["has_transfers"] == true && 
                item.contains("details") && item["details"].is_array()) {

                for (const auto& detail_item : item["details"]) {
                    if (detail_item.contains("is_transfer") && detail_item["is_transfer"] == true) {
                        continue;
                    }

                    auto seg = ParseSegment(detail_item);
                    if (!seg.from.title.empty() && !seg.to.title.empty() && seg.duration > 0) {
                        route.segments.push_back(seg);
                    }
                }
                
            } else {
                auto seg = ParseSegment(item);
                if (!seg.from.title.empty() && !seg.to.title.empty() && seg.duration > 0) {
                    route.segments.push_back(seg);
                }
            }
            
            if (route.segments.empty()) {
                continue;
            }
            
            route.CalculateSummary();
            
            if (route.total_duration <= 0 || route.total_duration > 7 * 24 * 3600) {
                continue;
            }
            
            routes.push_back(route);
            
        } catch (const std::exception& e) {
        }
    }
    
    return routes;
}

std::vector<Route> YandexAPI::FilterRoutes(const std::vector<Route>& routes, 
                                            int max_transfers, 
                                            int max_routes) const {
    std::vector<Route> filtered;
    
    for (const auto& route : routes) {
        int transfers = route.segments.empty() ? 0 : static_cast<int>(route.segments.size() - 1);
        if (transfers <= max_transfers) {
            filtered.push_back(route);
        }
    }
    
    std::sort(filtered.begin(), filtered.end(), [](const Route& a, const Route& b) {
        return a.total_duration < b.total_duration;
    });
    
    if (filtered.size() > static_cast<size_t>(max_routes)) {
        filtered.resize(max_routes);
    }
    
    return filtered;
}

std::vector<Route> YandexAPI::FindRoutes(const std::string& from, 
                                          const std::string& to, 
                                          const std::string& date,
                                          int max_routes,
                                          int max_transfers) {
    
    if (max_routes == -1) max_routes = default_routes_count_;
    
    std::string cache_key = MakeCacheKey(from, to, date);

    auto cached = cache_.Get(cache_key);
    if (cached.has_value()) {
        return FilterRoutes(cached.value(), max_transfers, max_routes);
    }
    
    std::vector<Route> all_routes;

    auto response = cpr::Get(
        cpr::Url{base_url_ + "/search/"},
        cpr::Parameters{
            {"apikey", api_key_},
            {"from", from},
            {"to", to},
            {"date", date},
            {"format", "json"},
            {"lang", "en_US"},
            {"transfers", "true"},
            {"limit", "100"},
            {"transport_types", "plane,train,suburban,bus,water"}
        }
    );
    
    if (response.status_code != 200) {
        return {};
    }
    
    try {
        auto data = json::parse(response.text);
        
        if (data.contains("error")) {
            return {};
        }
        
        all_routes = ParseRoutes(data);
        
        cache_.Put(cache_key, all_routes);
        
        return FilterRoutes(all_routes, max_transfers, max_routes);
        
    } catch (const std::exception& e) {
        return {};
    }
}

void YandexAPI::ShowCacheInfo() const {
    std::cout << "\nCache Information" << std::endl;
    std::cout << "  Entries: " << cache_.Size() << std::endl;
}

void YandexAPI::ClearCache() {
    cache_.Clear();
    std::cout << "Cache cleared" << std::endl;
}
