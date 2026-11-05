#ifndef HAMARC_H
#define HAMARC_H

#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "hamming.h"

namespace fs = std::filesystem;

class ArchiveIO {
 public:
  static void WriteByteEncoded(std::ostream& out, uint8_t raw);
  static uint8_t ReadByteEncoded(std::istream& in, bool* critical_error);
  static void WriteUInt64Encoded(std::ostream& out, uint64_t val);
  static uint64_t ReadUInt64Encoded(std::istream& in);
  static void WriteStringEncoded(std::ostream& out, const std::string& s);
  static std::string ReadStringEncoded(std::istream& in);
  static void EncodeStream(std::istream& in, std::ostream& out);
  static void DecodeStream(std::istream& in, std::ostream& out,
                           uint64_t raw_size);

 private:
  static const size_t kBufferSize;
};

class HamArc {
 public:
  HamArc() : header_signature_("HAF\0") {}

  void Create(const std::string& archive_name,
              const std::vector<std::string>& files);
  void List(const std::string& archive_name);
  void Extract(const std::string& archive_name,
               const std::vector<std::string>& files_to_extract);
  void Append(const std::string& archive_name,
              const std::vector<std::string>& new_files);
  void DeleteFile(const std::string& archive_name,
                  const std::string& file_to_delete);
  void Concatenate(const std::string& archive1, const std::string& archive2,
                   const std::string& output_name);

 private:
  void WriteHeader(std::ostream& out);
  bool CheckHeader(std::istream& in);

  const std::string header_signature_;
};

#endif
