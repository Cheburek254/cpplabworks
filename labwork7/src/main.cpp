#ifdef _WIN32
#include <windows.h>
#endif

#include <iostream>
#include <cstdlib>
#include <string>
#include "app.hpp"
#include <exception>
int main(int argc, char* argv[]) {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    try {
        const char* api_key = std::getenv("YANDEX_API_KEY");
        if (!api_key) {
            std::cerr << "Error: YANDEX_API_KEY environment variable not set" << std::endl;
            std::cerr << "   set YANDEX_API_KEY=your_key" << std::endl;
            return 1;
        }
        
        std::string home_city = "Penza";
        int default_routes = 3;
        
        for (int i = 1; i < argc; i++) {
            std::string arg = argv[i];
            if (arg == "-c" && i + 1 < argc) {
                home_city = argv[++i];
            } else if (arg == "-n" && i + 1 < argc) {
                default_routes = std::stoi(argv[++i]);
            } else if (arg == "-h" || arg == "--help") {
                std::cout << "Usage: " << argv[0] << " [-c city] [-n number]" << std::endl;
                return 0;
            }
        }
        
        App app(api_key, home_city, default_routes);
        app.Run();
        
    } catch (const std::exception& e) {
        std::cerr << "FATAL ERROR: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
