#include "app.hpp"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <string>
#include <cstddef>
#include <vector>
#include <exception>
App::App(const std::string& api_key, const std::string& home_city, int default_routes)
    : router_(api_key, home_city, default_routes), default_routes_count_(default_routes) {}

void App::PrintRouteSummary(const Route& route, int index) {
    std::cout << "\nRoute " << index << std::endl;
    std::cout << "Transfers: " << route.transfers_count << std::endl;
    
    int hours = route.total_duration / 3600;
    int minutes = (route.total_duration % 3600) / 60;
    std::cout << "Time: " << hours << " h " << minutes << " m" << std::endl;
    
    if (route.total_price > 0) {
        std::cout << "Price: " << std::fixed << std::setprecision(2) << route.total_price << " RUB" << std::endl;
    }
    
    if (!route.departure_time.empty() && !route.arrival_time.empty()) {
        std::cout << "Departure: " << route.departure_time.substr(0, 16) << std::endl;
        std::cout << "Arrival: " << route.arrival_time.substr(0, 16) << std::endl;
    }
}

void App::PrintRoute(const Route& route, int index) {
    PrintRouteSummary(route, index);
    
    for (size_t i = 0; i < route.segments.size(); ++i) {
        const auto& seg = route.segments[i];
        std::cout << "\n  Segment " << i + 1 << ":" << std::endl;
        std::cout << "    " << seg.from.title << " -> " << seg.to.title << std::endl;
        
        if (!seg.departure.empty()) {
            std::cout << "    Departure: " << seg.departure.substr(0, 16) << std::endl;
        }
        if (!seg.arrival.empty()) {
            std::cout << "    Arrival: " << seg.arrival.substr(0, 16) << std::endl;
        }
        
        std::cout << "    Transport: " << seg.thread.transport_type;
        if (!seg.thread.number.empty()) {
            std::cout << " " << seg.thread.number;
        }
        std::cout << std::endl;
        
        if (!seg.thread.carrier.title.empty()) {
            std::cout << "    Carrier: " << seg.thread.carrier.title << std::endl;
        }
        
        if (!seg.departure_terminal.empty() || !seg.arrival_terminal.empty()) {
            std::cout << "    Terminals: ";
            if (!seg.departure_terminal.empty()) {
                std::cout << "dep. " << seg.departure_terminal;
            }
            if (!seg.arrival_terminal.empty()) {
                if (!seg.departure_terminal.empty()) std::cout << ", ";
                std::cout << "arr. " << seg.arrival_terminal;
            }
            std::cout << std::endl;
        }
        
        float seg_price = seg.GetPrice();
        if (seg_price > 0) {
            std::cout << "    Price: " << std::fixed << std::setprecision(2) << seg_price << " RUB" << std::endl;
        }
    }
}

void App::HandleFindRoutes(const std::string& date, bool to_home, int max_routes, int max_transfers) {
    try {
        std::vector<Route> routes;
        
        if (to_home) {
            routes = router_.FindRoutesToHome(date, max_routes, max_transfers);
        } else {
            routes = router_.FindRoutesFromHome(date, max_routes, max_transfers);
        }
        
        if (routes.empty()) {
            std::cout << "No routes found" << std::endl;
            return;
        }
        
        std::cout << "\nFound " << routes.size() << " routes" << std::endl;
        
        for (size_t i = 0; i < routes.size(); ++i) {
            PrintRoute(routes[i], i + 1);
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}

void App::ShowCacheCommand() {
    router_.GetApi().ShowCacheInfo();
}

void App::ShowHelp() const {
    std::cout << "\nCommands:" << std::endl;
    std::cout << "  to_home YYYY-MM-DD [-n N] [-t M]   - Search routes from SPb to home" << std::endl;
    std::cout << "      N - number of routes to display (default: " << default_routes_count_ << ")" << std::endl;
    std::cout << "      M - max transfers (0 or 1, default: 1)" << std::endl;
    std::cout << "  from_home YYYY-MM-DD [-n N] [-t M] - Search routes from home to SPb" << std::endl;
    std::cout << "  cache                               - Show cache information" << std::endl;
    std::cout << "  clear                               - Clear route cache" << std::endl;
    std::cout << "  help                                - Show this help" << std::endl;
    std::cout << "  exit                                - Exit program" << std::endl;
}

void App::Run() {
    std::cout << "MADE BY YANDEX SCHEDULE API" << std::endl;
    std::cout << "Home city: " << router_.GetHomeCity() << std::endl;
    std::cout << "Default routes: " << default_routes_count_ << std::endl;
    std::cout << "Max transfers: 1" << std::endl;
    std::cout << "Cache: routes_cache.json (24h TTL)" << std::endl;
    std::cout << "Type 'help' for available commands" << std::endl;
    
    std::string line;
    while (running_) {
        std::cout << "\n> ";
        if (!std::getline(std::cin, line)) {
            break; 
        }
        
        if (line.empty()) continue;
        
        std::istringstream iss(line);
        std::string cmd;
        iss >> cmd;
        
        if (cmd == "exit" || cmd == "quit") {
            running_ = false;
            continue;
        }
        
        if (cmd == "help") {
            ShowHelp();
            continue;
        }
        
        if (cmd == "cache") {
            ShowCacheCommand();
            continue;
        }

        if (cmd == "clear") {
            router_.GetApi().ClearCache();
            continue;
        }
        
        if (cmd == "to_home" || cmd == "from_home") {
            std::string date;
            int n = default_routes_count_;
            int t = 1;
            
            iss >> date;
            
            std::string opt;
            while (iss >> opt) {
                if (opt == "-n" && iss >> n) {
                  
                } else if (opt == "-t" && iss >> t) {
                    if (t != 0 && t != 1) {
                        std::cout << "Transfers must be 0 or 1" << std::endl;
                        t = 1;
                    }
                }
            }
            
            if (!date.empty()) {
                HandleFindRoutes(date, cmd == "to_home", n, t);
            } else {
                std::cout << "Usage: " << cmd << " YYYY-MM-DD [-n N] [-t M]" << std::endl;
            }
            continue;
        }
        
        std::cout << "Unknown command. Type 'help' for see commands." << std::endl;
    }
}
