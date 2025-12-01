#include "hamarc.h"
#include "hamming.h"
#include <cstddef>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <map>
#include <cstring>
#include <cstdio>
#ifdef _WIN32
#include <windows.h>
#else
#include <sys/stat.h>
#include <unistd.h>
#endif

namespace hamarc{

std::vector<char> EncodeHeader(const FileHeader& header) {
    std::vector<char> raw_data(sizeof(FileHeader));
    std::memcpy(raw_data.data(), &header, sizeof(FileHeader));
    return hammingcoder::EncodeBuffer(raw_data.data(), raw_data.size());
}

FileHeader DecodeHeader(const char* encoded_data) {
    int correct, uncorrect;
    std::vector<char> decoded = hammingcoder::DecodeBuffer(encoded_data, sizeof(EncodedFileHeader), correct, uncorrect);
    FileHeader header;
    if (decoded.size() >= sizeof(FileHeader)) {
        std::memcpy(&header, decoded.data(), sizeof(FileHeader));
    }
    return header;
}

std::vector<char> EncodeFileEntry(const FileEntry& entry) {
    std::vector<char> raw_data(sizeof(FileEntry));
    std::memcpy(raw_data.data(), &entry, sizeof(FileEntry));
    return hammingcoder::EncodeBuffer(raw_data.data(), raw_data.size());
}

FileEntry DecodeFileEntry(const char* encoded_data) {
    int correct, uncorrect;
    std::vector<char> decoded = hammingcoder::DecodeBuffer(encoded_data, sizeof(EncodedFileEntry), correct, uncorrect);
    FileEntry entry;
    if (decoded.size() >= sizeof(FileEntry)) {
        std::memcpy(&entry, decoded.data(), sizeof(FileEntry));
    }
    return entry;
}

bool FileExist(const std::string& path){
    std::ifstream file(path);
    return file.good();
}

std::string GetFilename(const std::string& path){
    size_t pos = path.find_last_of("/\\");
    if (pos == std::string::npos){
        return path;
    }
    return path.substr(pos+1);
}

bool RenameFile(const std::string& old_path, const std::string& new_path){
    return std::rename(old_path.c_str(), new_path.c_str()) == 0;
}

bool MakeDirectory(const std::string& path){
    #ifdef _WIN32
        return CreateDirectoryA(path.c_str(), NULL) != 0;
    #else
        return mkdir(path.c_str(), 0755) == 0;
    #endif
}

bool ValidateArchive(std::ifstream& file, ArchiveState& state){
    EncodedFileHeader encoded_header;
    file.read(reinterpret_cast<char*>(&encoded_header), sizeof(encoded_header));
    
    if (file.gcount() != sizeof(encoded_header)) {
        return false;
    }
    FileHeader header = DecodeHeader(reinterpret_cast<const char*>(&encoded_header));
    
    if (std::string(header.magic, 4) != "HAF\x01") {
        return false;
    }

    state.files.clear();
    for (unsigned int i = 0; i < header.fileCount; i++) {
        EncodedFileEntry encoded_entry;
        file.read(reinterpret_cast<char*>(&encoded_entry), sizeof(encoded_entry));

        if (file.gcount() != sizeof(encoded_entry)) {
            return false;
        }

        FileEntry entry = DecodeFileEntry(reinterpret_cast<const char*>(&encoded_entry));
        state.files[entry.filename] = entry;
    }
    
    return true;
}

bool AddFileToArchive(const std::string &filePath, std::ofstream &archive, ArchiveState &state, unsigned long long &currentOffset){
    std::ifstream file(filePath, std::ios::binary);
    if (!file) {
        std::cout << "Cannot open file: " << filePath << std::endl;
        return false;
    }

    file.seekg(0, std::ios::end);
    size_t file_size = file.tellg();
    file.seekg(0, std::ios::beg);

    if (file_size == static_cast<size_t>(-1)) {
        return false;
    }

    std::vector<char> original_data(file_size);
    file.read(original_data.data(), file_size);

    std::vector<char> encoded_data = hammingcoder::EncodeData(original_data);

    FileEntry entry = {};
    std::string filename = GetFilename(filePath);
    std::strncpy(entry.filename, filename.c_str(), sizeof(entry.filename) - 1);
    entry.filename[sizeof(entry.filename) - 1] = '\0';
    entry.originalSize = original_data.size();
    entry.encodedSize = encoded_data.size();
    entry.offset = currentOffset;

    archive.seekp(currentOffset);
    archive.write(encoded_data.data(), encoded_data.size());

    if (!archive) {
        return false;
    }

    currentOffset += encoded_data.size();
    state.files[filename] = entry;

    return true;
}

bool CreateArchive(const std::string &archive_path, const std::vector<std::string> &file_paths){
    std::ofstream archive(archive_path, std::ios::binary);
    if (!archive) {
        return false;
    }

    ArchiveState state;
    state.archivePath = archive_path;
    
    FileHeader header = {};
    std::vector<char> encoded_header = EncodeHeader(header);
    archive.write(encoded_header.data(), encoded_header.size());
    unsigned long long current_offset = encoded_header.size();

    for (const auto& file_path : file_paths){
        FileEntry entry = {};
        std::string filename = GetFilename(file_path);
        std::strncpy(entry.filename, filename.c_str(), sizeof(entry.filename) - 1);
        std::vector<char> encoded_entry = EncodeFileEntry(entry);
        archive.write(encoded_entry.data(), encoded_entry.size());
        current_offset += encoded_entry.size();
    }

    for (const auto& file_path : file_paths){
        if (!AddFileToArchive(file_path, archive, state, current_offset)){
            return false;
        }
    }

    header.fileCount = state.files.size();
    header.totalSize = current_offset;
    std::vector<char> updated_header = EncodeHeader(header);
    archive.seekp(0);
    archive.write(updated_header.data(), updated_header.size());

    archive.seekp(encoded_header.size());
    for (const auto& [filename, entry] : state.files){
        std::vector<char> encoded_entry = EncodeFileEntry(entry);
        archive.write(encoded_entry.data(), encoded_entry.size());
    }

    return true;
}

bool LoadArchive(ArchiveState &state){
    std::ifstream archive(state.archivePath, std::ios::binary);
    if (!archive){
        return false;
    }

    return ValidateArchive(archive, state);
}

std::vector<std::string> ListFiles(const ArchiveState &state){
    std::vector<std::string> files;
    for (const auto& [filename, entry] : state.files){
        files.push_back(filename);
    }
    return files;
}

bool ExtractFile(const ArchiveState &state, const std::string &filename, const std::string& output){
    auto it = state.files.find(filename);
    if (it == state.files.end()){
        return false;
    }

    std::ifstream archive(state.archivePath, std::ios::binary);
    if(!archive){
        return false;
    }

    const FileEntry& entry = it -> second;
    
    archive.seekg(entry.offset);
    std::vector<char> encoded_data(entry.encodedSize);
    archive.read(encoded_data.data(), entry.encodedSize);
    
    if (archive.gcount() != static_cast<std::streamsize>(entry.encodedSize)) {
        return false;
    }

    std::string out_file = output.empty() ? filename : output;
    std::ofstream output_file(out_file, std::ios::binary);
    if (!output_file){
        return false;
    }

    int correct, uncorrect;
    std::vector<char> decoded_data = hammingcoder::DecodeData(encoded_data, correct, uncorrect);

    if (uncorrect > 0 ){
        std::cout << "ФАЙЛ УВЫ ПОВРЕЖДЕН ПЛАКИ ПЛАКИ :((()))";
        return false;
    }

    output_file.write(decoded_data.data(), decoded_data.size());
    
    return true;
}

bool ExtractAll(const ArchiveState &state, const std::string &output_dir){

    if (!output_dir.empty() && !FileExist(output_dir)){
        if (!MakeDirectory(output_dir)){
            return false;
        }
    }

    for (const auto& [filename, entry] : state.files){
        std::string output_path = output_dir.empty() ? filename : (output_dir + "/" + filename);
        if (!ExtractFile(state, filename, output_path)){
            return false;
        }
    }
    return true;
}

bool AppendFile(ArchiveState &state, const std::string &filePath){
    std::fstream archive(state.archivePath, std::ios::binary | std::ios::in | std::ios::out);
    if (!archive){
        return false;
    }
    
    if (!LoadArchive(state)) {
        return false;
    }

    FileHeader header;
    archive.read(reinterpret_cast<char*>(&header), sizeof(header));
    if (!archive) {
        return false;
    }

    archive.seekp(0, std::ios::end);
    unsigned long long current_offset = archive.tellp();

    std::ifstream file(filePath, std::ios::binary);
    if(!file){
        return false;
    }

    file.seekg(0, std::ios::end);
    size_t file_size = file.tellg();
    file.seekg(0, std::ios::beg);

    if (file_size == static_cast<size_t>(-1)) {
        return false;
    }

    std::vector<char> file_data;
    const size_t chunk_size = 64 * 1024; 
    std::vector<char> chunk(chunk_size);
    
    while (file.read(chunk.data(), chunk_size) || file.gcount() > 0) {
        size_t bytes_read = file.gcount();
        file_data.insert(file_data.end(), chunk.begin(), chunk.begin() + bytes_read);
    }

    if (file_data.size() != file_size) {
        return false;
    }

    std::vector<char> encoded_data = hammingcoder::EncodeData(file_data);

    FileEntry new_entry = {};
    std::string filename = GetFilename(filePath);
    std::strncpy(new_entry.filename, filename.c_str(), sizeof(new_entry.filename) - 1);
    new_entry.filename[sizeof(new_entry.filename) - 1] = '\0';
    new_entry.originalSize = file_data.size();
    new_entry.encodedSize = encoded_data.size();
    new_entry.offset = current_offset;

    archive.write(encoded_data.data(), encoded_data.size());
    if (!archive) {
        return false;
    }

    state.files[filename] = new_entry;
    header.fileCount = state.files.size();
    header.totalSize = current_offset + encoded_data.size();

    archive.seekp(0);
    archive.write(reinterpret_cast<const char*>(&header), sizeof(header));
    if (!archive) {
        return false;
    }

    archive.seekp(sizeof(FileHeader));
    for (const auto& [name, file_entry] : state.files){
        archive.write(reinterpret_cast<const char*>(&file_entry), sizeof(file_entry));
        if (!archive) {
            return false;
        }
    }

    return true;
}

bool KillFile(ArchiveState &state, const std::string &filename){
    auto it = state.files.find(filename);
    if (it == state.files.end()){
        return false;
    }

    state.files.erase(it);

    std::vector<std::string> temp;
    for (const auto& [name, entry] : state.files){
        temp.push_back(name);
    }
    std::string temp_path = state.archivePath + ".tmp";
    if (!CreateArchive(temp_path, temp)){
        return false;
    }
    if(!RenameFile(temp_path, state.archivePath)){
        return false;
    }

    ArchiveState new_state;
    new_state.archivePath = state.archivePath;
    if(!LoadArchive(new_state)){
        return false;
    }
    state.files = new_state.files;
    return true;
}

bool ConcatenateArchives(const std::string &archive1, const std::string &archive2, const std::string &output_archive){
    ArchiveState state1, state2;
    state1.archivePath = archive1;
    state2.archivePath = archive2;

    if(!LoadArchive(state1) || !LoadArchive(state2)){
        return false;
    }
    auto files1 = ListFiles(state1);
    auto files2 = ListFiles(state2);

    std::vector<std::string> all_files;
    all_files.insert(all_files.end(), files1.begin(), files1.end());
    all_files.insert(all_files.end(), files2.begin(), files2.end());
    return CreateArchive(output_archive, all_files);
}

void PrintArchiveInfo(const ArchiveState &state){
    int i = 1;
    for (const auto& [filename, entry] : state.files){
        std::cout << i << ": " << filename << std::endl;
        i++;
    }
}

}// namespace hamarc
