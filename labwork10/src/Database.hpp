#pragma once

#include "Types.hpp"
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace valkey {

class Database {
public:
  Database() = default;

  std::string ProcessCommand(const std::vector<std::string>& args);

private:
  std::unordered_map<std::string, DbEntry> storage_;
  uint64_t max_memory_ = 0;
  uint64_t current_memory_ = 0;

  void DeleteKey(const std::string& key);
  bool IsExpired(const std::string& key);
  void CheckExpirations();
  uint64_t EstimateMemory(const std::string& key, const DbEntry& entry) const;
  bool CheckMemoryLimit(uint64_t expected_additional) const;
  void UpdateKey(const std::string& key, const ValueVariant& val);

  std::string ProcessString(const std::string& cmd,
                            const std::vector<std::string>& args);
  std::string ProcessList(const std::string& cmd,
                          const std::vector<std::string>& args);
  std::string ProcessSet(const std::string& cmd,
                         const std::vector<std::string>& args);
  std::string ProcessGeo(const std::string& cmd,
                         const std::vector<std::string>& args);
  std::string ProcessGeneral(const std::string& cmd,
                             const std::vector<std::string>& args);

  std::string FormatInt(int64_t val) const;
  std::string FormatString(const std::string& val) const;
  std::string FormatArray(const std::vector<std::string>& arr) const;
  std::string FormatError(const std::string& msg) const;
  std::string FormatNil() const;
  std::string FormatOk() const;
};

} // namespace valkey