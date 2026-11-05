#pragma once

#include "router.hpp"
#include <string>

class App {
private:
    Router router_;
    bool running_ = true;
    int default_routes_count_;

public:
    void PrintRoute(const Route& route, int index);
    void PrintRouteSummary(const Route& route, int index);
    void HandleFindRoutes(const std::string& date, bool to_home, int max_routes, int max_transfers);
    void ShowCacheCommand();
    void ShowHelp() const;

public:
    App(const std::string& api_key, const std::string& home_city, int default_routes = 3);
    
    void Run();
};