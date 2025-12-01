#ifndef HAMARC_H
#define HAMARC_H

#include <string>
#include <vector>
#include <map>
#include <fstream>

namespace hamarc {

#pragma pack(push, 1)
struct FileHeader {
    char magic[4] = {'H', 'A', 'F', '\x01'};
    unsigned int fileCount;
    unsigned long long totalSize;
};

struct FileEntry {
    char filename[256];
    unsigned long long originalSize;
    unsigned long long encodedSize;
    unsigned long long offset;
};
struct EncodedFileHeader {
    char encoded_magic[8];    
    char encoded_fileCount[8];  
    char encoded_totalSize[16]; 
};

struct EncodedFileEntry {
    char encoded_filename[512];   
    char encoded_originalSize[16];
    char encoded_encodedSize[16];
    char encoded_offset[16];
};
#pragma pack(pop)

struct ArchiveState {
    std::string archivePath;
    std::map<std::string, FileEntry> files;
};

bool CreateArchive(const std::string& archive_path, const std::vector<std::string>& file_paths);
bool LoadArchive(ArchiveState& state);
std::vector<std::string> ListFiles(const ArchiveState& state);
bool ExtractFile(const ArchiveState& state, const std::string& filename, const std::string& output_path);
bool ExtractAll(const ArchiveState& state, const std::string& output_dir);
bool ArchiveStateppendFile(ArchiveState& state, const std::string& file_path);
bool KillFile(ArchiveState& state, const std::string& filename);
bool ConcatenateArchives(const std::string& archive1, const std::string& archive2, const std::string& output_archive);
void PrintArchiveInfo(const ArchiveState& state);
bool ValidateArchive(std::ifstream& file, ArchiveState& state);
bool AppendFile(ArchiveState& state, const std::string& file_path);
std::vector<char> EncodeHeader(const FileHeader& header);
FileHeader DecodeHeader(const char* encoded_data);
std::vector<char> EncodeFileEntry(const FileEntry& entry);
FileEntry DecodeFileEntry(const char* encoded_data);
}// namespace HamArc
#endif
