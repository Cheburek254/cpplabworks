#include "Database.hpp"
#include <cctype>
#include <iostream>
#include <algorithm>
#include <sstream>
#include <string>
#include <vector>

int main(int argc, char** argv) {
  valkey::Database db;
  for (int i = 1; i < argc; ++i) {
    if (std::string(argv[i]) == "--maxmemory" && i + 1 < argc) {
      db.ProcessCommand({"CONFIG", "SET", "maxmemory", argv[i + 1]});
      i++;
    }
  }

  std::string line;
  while (std::getline(std::cin, line)) {
    std::vector<std::string> args;
    std::istringstream iss(line);
    std::string token;
    while (iss >> token) args.push_back(token);
    if (args.empty()) continue;

    std::string first_cmd = args[0];
    std::transform(first_cmd.begin(), first_cmd.end(), first_cmd.begin(),
                   [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
    if (first_cmd == "EXIT") {
      break;
    }

    std::string res = db.ProcessCommand(args);
    if (!res.empty()) {
      if (res.starts_with("(error)")) std::cerr << res << std::endl;
      else std::cout << res << std::endl;
    }
  }
  return 0;
}