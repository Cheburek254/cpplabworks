#include <iostream>
#include <string>
#include <exception>
#include "proj.h"

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Unknown command, using: ./itmoloops score.txt music.wav"
                  << std::endl;
        return 1;
    }

    try {
        Project proj;
        proj.LoadFromFile(argv[1]);
        proj.Render(argv[2]);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
