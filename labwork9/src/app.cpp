#include "TTaskScheduler.h"
#include <iostream>
#include <vector>
#include <string>
#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <stdexcept>
#include <cpr/cpr.h>
#include <nlohmann/json.hpp>
#include <algorithm>

#ifdef _WIN32
#include <windows.h>
#endif

using json = nlohmann::json;

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

struct Location {
    double lat;
    double lon;
    std::string city;
};

struct WeatherInfo {
    double temperature;
    bool is_raining;
    std::string condition;
};

struct Place {
    std::string name;
    std::string type;
    double lat;
    double lon;
};

struct Recommendation {
    std::string place_name;
    std::string type;
    double distance_km;
};

Location fetchLocation() {
    std::cout << "[API] Requesting location from ip-api.com..." << std::endl;
    cpr::Response r = cpr::Get(
        cpr::Url{"http://ip-api.com/json/"},
        cpr::Timeout{5000},
        cpr::Header{{"User-Agent", "App/1.0"}}
    );
    if (r.status_code != 200) {
        throw std::runtime_error("Failed to fetch location. HTTP Status: " + std::to_string(r.status_code) + " | Error: " + r.error.message);
    }
    
    auto j = json::parse(r.text);
    if (j.contains("status") && j["status"] == "fail") {
        throw std::runtime_error("ip-api error: " + j["message"].get<std::string>());
    }
    double lat = j["lat"].get<double>();
    double lon = j["lon"].get<double>();
    std::string city = j["city"].get<std::string>();
    
    std::cout << "[Info] Location detected: " << city << " (" << lat << ", " << lon << ")" << std::endl;
    return {lat, lon, city};
}

WeatherInfo fetchWeather(const Location& loc) {
    std::cout << "[API] Requesting weather for " << loc.city << " from Open-Meteo..." << std::endl;
    std::string url = "https://api.open-meteo.com/v1/forecast?latitude=" + std::to_string(loc.lat) +
                      "&longitude=" + std::to_string(loc.lon) + "&current=temperature_2m,precipitation";
                      
    cpr::Response r = cpr::Get(
        cpr::Url{url},
        cpr::Timeout{5000},
        cpr::VerifySsl{false},
        cpr::Header{{"User-Agent", "App/1.0"}}
    );
    if (r.status_code != 200) throw std::runtime_error("Failed to fetch weather. HTTP Status: " + std::to_string(r.status_code) + " | Error: " + r.error.message);

    auto j = json::parse(r.text);
    double temp = j["current"]["temperature_2m"];
    double precip = j["current"]["precipitation"];
    
    bool is_raining = precip > 0.0;
    std::cout << "[Info] Weather: " << temp << "°C, " << (is_raining ? "Rain" : "Clear") << std::endl;
    return {temp, is_raining, is_raining ? "Rain" : "Clear"};
}

std::vector<Place> fetchPlaces(const Location& loc, const WeatherInfo& weather, const char* key) {
    std::cout << "[API] Requesting nearby places from 2GIS API..." << std::endl;
    
    std::string query = weather.is_raining ? "кафе, кофейни" : "парки, скверы, сады";
    cpr::Response r = cpr::Get(
        cpr::Url{"https://catalog.api.2gis.com/3.0/items"},
        cpr::Parameters{
            {"q", query},
            {"point", std::to_string(loc.lon) + "," + std::to_string(loc.lat)},
            {"radius", "750"},
            {"fields", "items.point,items.rubrics"},
            {"page_size", "10"},
            {"key", key}
        },
        cpr::Timeout{5000},
        cpr::VerifySsl{false},
        cpr::Header{{"User-Agent", "App/1.0"}}
    );
    if (r.status_code != 200) throw std::runtime_error("Failed to fetch places from 2GIS API. HTTP Status: " + std::to_string(r.status_code) + " | Error: " + r.error.message);

    std::vector<Place> places;
    auto j = json::parse(r.text);
    
    if (!j.contains("result") || !j["result"].contains("items")) {
        std::cout << "[Warning] 2GIS API returned no items." << std::endl;
        return places;
    }
    
    for (const auto& item : j["result"]["items"]) {
        std::string name = item.contains("name") ? item["name"].get<std::string>() : "Неизвестное место";
        std::string type = "без категории";
        if (item.contains("rubrics") && !item["rubrics"].empty()) {
            type = item["rubrics"][0]["name"].get<std::string>();
        }
        
        if (item.contains("point")) {
            places.push_back({name, type, item["point"]["lat"], item["point"]["lon"]});
        }
    }
    
    std::cout << "[Info] Found " << places.size() << " places nearby." << std::endl;
    return places;
}

double calculateDistance(double lat1, double lon1, double lat2, double lon2) {
    constexpr double R = 6371.0; 
    double dLat = (lat2 - lat1) * M_PI / 180.0;
    double dLon = (lon2 - lon1) * M_PI / 180.0;
    lat1 = lat1 * M_PI / 180.0;
    lat2 = lat2 * M_PI / 180.0;

    double a = std::pow(std::sin(dLat / 2), 2) + std::pow(std::sin(dLon / 2), 2) * std::cos(lat1) * std::cos(lat2);
    double c = 2 * std::asin(std::sqrt(a));
    return R * c;
}

int main() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    const char* key = std::getenv("TWOGIS_API_KEY");
    if (!key || !*key) {
        std::cerr << "Set TWOGIS_API_KEY before running the application.\n";
        return 1;
    }

    try {
        TTaskScheduler scheduler;

        auto locTask = scheduler.add(fetchLocation);
        auto weatherTask = scheduler.add(fetchWeather, locTask.getFutureResult<const Location&>());
        auto placesTask = scheduler.add(fetchPlaces, locTask.getFutureResult<const Location&>(), weatherTask.getFutureResult<const WeatherInfo&>(), key);
        
        auto recommendTask = scheduler.add([](std::vector<Place>&& places, const Location& loc) {
            std::vector<Recommendation> recs;
            for (const auto& place : places) {
                double dist = calculateDistance(loc.lat, loc.lon, place.lat, place.lon);
                recs.push_back({place.name, place.type, dist});
            }
            return recs;
        }, placesTask.getFutureResult<std::vector<Place>>(), locTask.getFutureResult<const Location&>());

        recommendTask.apply([](std::vector<Recommendation>&& recommendations) {
            std::cout << "\nRecomendations\n";
            if (recommendations.empty()) {
                std::cout << "No recommendations found for your current weather and location.\n";
                return;
            }
            size_t limit = std::min<size_t>(recommendations.size(), 10);
            for (size_t i = 0; i < limit; ++i) {
                const auto& rec = recommendations[i];
                std::cout << "- " << rec.place_name << " [" << rec.type << "] (" << std::fixed << std::setprecision(2) << rec.distance_km << " km away)\n";
            }
        });

        scheduler.executeAll();
    } catch (const std::exception& e) {
        std::cerr << "\n[FATAL ERROR] " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
