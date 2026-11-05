#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "hamarc.h"

void PrintUsage() {
  std::cout << "Usage: hamarc [OPTIONS] [FILES...]\n";
  std::cout << "  -c, --create            Create new archive\n";
  std::cout << "  -f, --file=[ARCH]       Specify archive filename\n";
  std::cout << "  -l, --list              List files in archive\n";
  std::cout << "  -x, --extract           Extract files\n";
  std::cout << "  -a, --append            Append files to archive\n";
  std::cout << "  -d, --delete            Delete file from archive\n";
  std::cout << "  -A, --concatenate       Merge two archives\n";
}

int main(int argc, char* argv[]) {
  if (argc < 2) {
    PrintUsage();
    return 1;
  }

  std::string archive_name;
  std::vector<std::string> file_args;
  std::string mode;

  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg == "-c" || arg == "--create") {
      mode = "create";
    } else if (arg == "-l" || arg == "--list") {
      mode = "list";
    } else if (arg == "-x" || arg == "--extract") {
      mode = "extract";
    } else if (arg == "-a" || arg == "--append") {
      mode = "append";
    } else if (arg == "-d" || arg == "--delete") {
      mode = "delete";
    } else if (arg == "-A" || arg == "--concatenate") {
      mode = "concat";
    } else if (arg.rfind("-f", 0) == 0 || arg.rfind("--file=", 0) == 0) {
      if (arg == "-f") {
        if (i + 1 < argc) archive_name = argv[++i];
      } else if (arg.rfind("--file=", 0) == 0) {
        archive_name = arg.substr(7);
      }
    } else {
      file_args.push_back(arg);
    }
  }

  HamArc app;
  try {
    if (mode == "create") {
      if (archive_name.empty())
        throw std::runtime_error("Need archive name (-f)");
      app.Create(archive_name, file_args);
    } else if (mode == "list") {
      if (archive_name.empty())
        throw std::runtime_error("Need archive name (-f)");
      app.List(archive_name);
    } else if (mode == "extract") {
      if (archive_name.empty())
        throw std::runtime_error("Need archive name (-f)");
      app.Extract(archive_name, file_args);
    } else if (mode == "append") {
      if (archive_name.empty())
        throw std::runtime_error("Need archive name (-f)");
      app.Append(archive_name, file_args);
    } else if (mode == "delete") {
      if (archive_name.empty())
        throw std::runtime_error("Need archive name (-f)");
      if (file_args.empty())
        throw std::runtime_error("Need deletion file name");
      app.DeleteFile(archive_name, file_args[0]);
    } else if (mode == "concat") {
      if (file_args.size() < 2)
        throw std::runtime_error("Need 2 archives");
      if (archive_name.empty())
        throw std::runtime_error("Need output archive (-f)");
      app.Concatenate(file_args[0], file_args[1], archive_name);
    } else {
      PrintUsage();
    }
  } catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << "\n";
    return 1;
  }

  return 0;
}
