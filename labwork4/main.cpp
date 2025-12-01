#include "hamarc.h"
#include <vector>
#include <string>
#include <cstddef>
int main(int argc, char* argv[]) {
	

	std::vector<std::string> args(argv+1, argv+argc);
	std::string archive_path;
	std::vector<std::string> files;
	for (size_t i = 0; i < args.size(); i++){
		if (args[i] == "-f" || args[i] == "--file"){
			if (i + 1 < args.size()){
				archive_path = args[++i];
			}
		}
		else if (args[i].find("--file=") == 0){
			archive_path = args[i].substr(std::string("--file=").size());

		} 
		else if (args[i] != "-c" && args[i] != "-l" && args[i] != "-x" && 
                args[i] != "-a" && args[i] != "-d" && args[i] != "-A" &&
                args[i] != "--create" && args[i] != "--list" && args[i] != "--extract" &&
                args[i] != "--append" && args[i] != "--delete" && args[i] != "--concatenate"){
					files.push_back(args[i]);
				   }
	}
	
	hamarc::ArchiveState state;
	state.archivePath = archive_path;

	std::string command = args[0];
	if (command == "-c" || command == "--create"){
		
		hamarc::CreateArchive(archive_path, files);
	}
	else if (command == "-l" || command == "--list"){
		if (hamarc::LoadArchive(state)){
			hamarc::PrintArchiveInfo(state);
		}
	}
	else if (command == "-x" || command == "--extract"){
		if (!hamarc::LoadArchive(state)){
			return 1;
		}
		if (files.empty()){
			hamarc::ExtractAll(state, std::string(""));

		} else{
			for (const auto& file : files){
				hamarc::ExtractFile(state, file, std::string("out"));

			}
		}

	}
	else if (command == "-a" || command == "--append"){
		
		
		for (const auto& file : files){
			hamarc::AppendFile(state, file);
		}
	}
	else if (command == "-d" || command == "--delete"){
		
		for (const auto& file : files){
			hamarc::KillFile(state, file);
		}
	}
	else if (command == "-A" || command == "--concatenate"){
		
		hamarc::ConcatenateArchives(files[0], files[1], archive_path);
	}
	return 0;
}
