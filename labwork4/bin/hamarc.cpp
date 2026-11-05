
#include "hamarc.h"

#include <algorithm>
#include <iostream>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>



const size_t ArchiveIO::kBufferSize = 4096;

void ArchiveIO::WriteByteEncoded(std::ostream& out, uint8_t raw) {
  uint8_t low = raw & 0x0F;
  uint8_t high = (raw >> 4) & 0x0F;
  uint8_t enc1 = HammingCodec::EncodeNibble(low);
  uint8_t enc2 = HammingCodec::EncodeNibble(high);
  out.put(static_cast<char>(enc1));
  out.put(static_cast<char>(enc2));
}

uint8_t ArchiveIO::ReadByteEncoded(std::istream& in, bool* critical_error) {
  char buf[2];
  if (!in.read(buf, 2)) throw std::runtime_error("Unexpected EOF");

  auto res1 = HammingCodec::DecodeByte(static_cast<uint8_t>(buf[0]));
  auto res2 = HammingCodec::DecodeByte(static_cast<uint8_t>(buf[1]));

  if (critical_error != nullptr) {
    if (res1.status == HammingCodec::Status::kDoubleError ||
        res2.status == HammingCodec::Status::kDoubleError) {
      *critical_error = true;
      std::cerr << "CRITICAL ERROR: Double bit error detected! "
                << "Archive data corruption.\n";
    }
  }

  return (res1.data) | (res2.data << 4);
}

void ArchiveIO::WriteUInt64Encoded(std::ostream& out, uint64_t val) {
  for (int i = 0; i < 8; ++i) {
    WriteByteEncoded(out, (val >> (i * 8)) & 0xFF);
  }
}

uint64_t ArchiveIO::ReadUInt64Encoded(std::istream& in) {
  uint64_t val = 0;
  bool err = false;
  for (int i = 0; i < 8; ++i) {
    uint64_t byte = ReadByteEncoded(in, &err);
    val |= (byte << (i * 8));
  }
  return val;
}

void ArchiveIO::WriteStringEncoded(std::ostream& out, const std::string& s) {
  uint16_t len = static_cast<uint16_t>(s.length());
  WriteByteEncoded(out, len & 0xFF);
  WriteByteEncoded(out, (len >> 8) & 0xFF);
  for (char c : s) WriteByteEncoded(out, static_cast<uint8_t>(c));
}

std::string ArchiveIO::ReadStringEncoded(std::istream& in) {
  bool err = false;
  uint16_t len = ReadByteEncoded(in, &err);
  len |= (ReadByteEncoded(in, &err) << 8);

  std::string s;
  s.reserve(len);
  for (int i = 0; i < len; ++i) {
    s += static_cast<char>(ReadByteEncoded(in, &err));
  }
  return s;
}

void ArchiveIO::EncodeStream(std::istream& in, std::ostream& out) {
  char in_buf[kBufferSize];
  while (in.read(in_buf, kBufferSize) || in.gcount() > 0) {
    size_t count = in.gcount();
    for (size_t i = 0; i < count; ++i) {
      WriteByteEncoded(out, static_cast<uint8_t>(in_buf[i]));
    }
    if (in.eof()) break;
  }
}

void ArchiveIO::DecodeStream(std::istream& in, std::ostream& out,
                             uint64_t raw_size) {
  uint64_t processed = 0;
  bool err = false;
  while (processed < raw_size) {
    uint8_t byte = ReadByteEncoded(in, &err);
    out.put(static_cast<char>(byte));
    ++processed;
  }
}

void HamArc::WriteHeader(std::ostream& out) {
  out.write(header_signature_.c_str(), 4);
}

bool HamArc::CheckHeader(std::istream& in) {
  char buf[4];
  if (!in.read(buf, 4)) return false;
  return std::memcmp(buf, header_signature_.c_str(), 4) == 0;
}

void HamArc::Create(const std::string& archive_name,
                    const std::vector<std::string>& files) {
  std::ofstream out(archive_name, std::ios::binary);
  if (!out) throw std::runtime_error("Cannot create archive file");

  WriteHeader(out);

  for (const auto& file_name : files) {
    if (!fs::exists(file_name)) {
      std::cerr << "Warning: File " << file_name << " not found. Skipping.\n";
      continue;
    }

    uint64_t file_size = fs::file_size(file_name);
    std::string name_only = fs::path(file_name).filename().string();

    ArchiveIO::WriteStringEncoded(out, name_only);
    ArchiveIO::WriteUInt64Encoded(out, file_size);

    std::ifstream in_file(file_name, std::ios::binary);
    ArchiveIO::EncodeStream(in_file, out);

    std::cout << "Added: " << name_only << "\n";
  }
  std::cout << "Archive created successfully.\n";
}

void HamArc::List(const std::string& archive_name) {
  std::ifstream in(archive_name, std::ios::binary);
  if (!in || !CheckHeader(in))
    throw std::runtime_error("Invalid archive or file not found");

  std::cout << "Files in " << archive_name << ":\n";
  while (in.peek() != EOF) {
    try {
      std::string name = ArchiveIO::ReadStringEncoded(in);
      uint64_t size = ArchiveIO::ReadUInt64Encoded(in);
      std::cout << "  " << name << " (" << size << " bytes)\n";

      in.seekg(size * 2, std::ios::cur);
    } catch (...) {
      break;
    }
  }
}

void HamArc::Extract(const std::string& archive_name,
                     const std::vector<std::string>& files_to_extract) {
  std::ifstream in(archive_name, std::ios::binary);
  if (!in || !CheckHeader(in)) throw std::runtime_error("Invalid archive");

  bool extract_all = files_to_extract.empty();

  while (in.peek() != EOF) {
    std::string name = ArchiveIO::ReadStringEncoded(in);
    uint64_t size = ArchiveIO::ReadUInt64Encoded(in);

    bool need_this = extract_all;
    if (!extract_all) {
      for (const auto& f : files_to_extract) {
        if (f == name) need_this = true;
      }
    }

    if (need_this) {
      std::cout << "Extracting: " << name << "... ";
      std::ofstream out_file(name, std::ios::binary);
      ArchiveIO::DecodeStream(in, out_file, size);
      std::cout << "OK\n";
    } else {
      in.seekg(size * 2, std::ios::cur);
    }
  }
}

void HamArc::Append(const std::string& archive_name,
                    const std::vector<std::string>& new_files) {
  if (!fs::exists(archive_name)) {
    Create(archive_name, new_files);
    return;
  }

  std::fstream out(archive_name, std::ios::binary | std::ios::in |
                                     std::ios::out);
  if (!out) throw std::runtime_error("Cannot open archive");

  out.seekp(0, std::ios::end);

  for (const auto& file_name : new_files) {
    if (!fs::exists(file_name)) continue;
    uint64_t file_size = fs::file_size(file_name);
    std::string name_only = fs::path(file_name).filename().string();

    ArchiveIO::WriteStringEncoded(out, name_only);
    ArchiveIO::WriteUInt64Encoded(out, file_size);

    std::ifstream in_file(file_name, std::ios::binary);
    ArchiveIO::EncodeStream(in_file, out);
    std::cout << "Appended: " << name_only << "\n";
  }
}

void HamArc::DeleteFile(const std::string& archive_name,
                        const std::string& file_to_delete) {
  std::ifstream in(archive_name, std::ios::binary);
  if (!in || !CheckHeader(in)) throw std::runtime_error("Invalid archive");

  std::string temp_name = archive_name + ".tmp";
  std::ofstream out(temp_name, std::ios::binary);
  WriteHeader(out);

  bool found = false;
  while (in.peek() != EOF) {
    std::string name = ArchiveIO::ReadStringEncoded(in);
    uint64_t size = ArchiveIO::ReadUInt64Encoded(in);

    if (name == file_to_delete) {
      found = true;
      in.seekg(size * 2, std::ios::cur);
      std::cout << "Deleted: " << name << "\n";
    } else {
      ArchiveIO::WriteStringEncoded(out, name);
      ArchiveIO::WriteUInt64Encoded(out, size);

      char buf[4096];
      uint64_t remaining = size * 2;
      while (remaining > 0) {
        uint64_t chunk = std::min(static_cast<uint64_t>(sizeof(buf)),
                                  remaining);
        in.read(buf, chunk);
        out.write(buf, chunk);
        remaining -= chunk;
      }
    }
  }
  in.close();
  out.close();

  if (found) {
    fs::remove(archive_name);
    fs::rename(temp_name, archive_name);
  } else {
    fs::remove(temp_name);
    std::cout << "File not found in archive.\n";
  }
}

void HamArc::Concatenate(const std::string& archive1,
                         const std::string& archive2,
                         const std::string& output_name) {
  std::ofstream out(output_name, std::ios::binary);
  WriteHeader(out);

  auto copy_content = [&](const std::string& src) {
    std::ifstream in(src, std::ios::binary);
    if (!in || !CheckHeader(in))
      throw std::runtime_error("Invalid source archive: " + src);

    out << in.rdbuf();
  };

  copy_content(archive1);
  copy_content(archive2);
  
}
