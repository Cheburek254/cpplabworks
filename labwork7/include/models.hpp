#pragma once

#include <string>
#include <vector>
#include <algorithm>
#include <nlohmann/json.hpp> // NOLINT(misc-include-cleaner)

// NOLINTBEGIN(readability-identifier-naming)

#ifdef _WIN32
#define NOMINMAX
#undef min
#undef max
#endif

using json = nlohmann::json;

struct Station {
    std::string code;
    std::string title;
    std::string popular_title;
    std::string short_title;
    std::string transport_type;
    std::string type;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(Station, code, title, popular_title, 
                                   short_title, transport_type, type)
};

struct Price {
    int cents;
    int whole;
    std::string currency;

    Price() : cents(0), whole(0) {}
    
    float GetTotal() const {
        return whole + cents / 100.0f;
    }

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(Price, cents, whole, currency)
};

struct Place {
    std::string name;
    Price price;
    std::string currency;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(Place, name, price, currency)
};

struct TicketsInfo {
    bool et_marker;
    std::vector<Place> places;

    float GetMinPrice() const {
        if (places.empty()) return 0.0f;
        
        float min_price = places[0].price.GetTotal();
        for (size_t i = 1; i < places.size(); ++i) {
            float current = places[i].price.GetTotal();
            if (current < min_price) {
                min_price = current;
            }
        }
        return min_price;
    }

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(TicketsInfo, et_marker, places)
};

struct Carrier {
    int code;
    std::string title;
    std::string url;
    std::string phone;
    std::string contacts;
    std::string logo;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(Carrier, code, title, url, phone, contacts, logo)
};

struct Thread {
    std::string uid;
    std::string title;
    std::string number;
    std::string short_title;
    std::string transport_type;
    std::string vehicle;
    Carrier carrier;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(Thread, uid, title, number, short_title, 
                                   transport_type, vehicle, carrier)
};

struct Segment {
    Station from;
    Station to;
    std::string departure;
    std::string arrival;
    int duration;
    bool has_transfers;
    Thread thread;
    TicketsInfo tickets_info;
    bool has_tickets_info = false;  
    std::string departure_terminal;
    std::string arrival_terminal;
    std::string departure_platform;
    std::string arrival_platform;
    std::string stops;

    float GetPrice() const {
        if (has_tickets_info) {
            return tickets_info.GetMinPrice();
        }
        return 0.0f;
    }

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(Segment, from, to, departure, arrival, duration,
                                   has_transfers, thread, tickets_info, has_tickets_info,
                                   departure_terminal, arrival_terminal, 
                                   departure_platform, arrival_platform, stops)
};

struct Route {
    std::vector<Segment> segments;
    int total_duration;
    int transfers_count;
    float total_price;
    std::string departure_time;
    std::string arrival_time;

    void CalculateSummary() {
        total_duration = 0;
        total_price = 0;
        
        if (!segments.empty()) {
            if (!segments[0].departure.empty()) {
                departure_time = segments[0].departure;
            }
            
            if (!segments.back().arrival.empty()) {
                arrival_time = segments.back().arrival;
            }
            
            for (const auto& seg : segments) {
                if (seg.duration > 0 && seg.duration < 86400 * 7) {
                    total_duration += seg.duration;
                }
                total_price += seg.GetPrice();
            }
        }
        
        transfers_count = segments.empty() ? 0 : static_cast<int>(segments.size() - 1);
    }

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(Route, segments, total_duration, transfers_count, 
                                   total_price, departure_time, arrival_time)
};

// NOLINTEND(readability-identifier-naming)
