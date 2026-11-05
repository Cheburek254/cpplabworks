#include "Database.hpp"
#include "Geo.hpp"
#include "Types.hpp"
#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iterator>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace {
bool Match(const std::string& pattern, const std::string& str) {
  size_t p = 0, s = 0;
  size_t star_idx = std::string::npos;
  size_t match_idx = 0;
  while (s < str.size()) {
    if (p < pattern.size() && (pattern[p] == '?' || pattern[p] == str[s])) {
      p++; s++;
    } else if (p < pattern.size() && pattern[p] == '*') {
      star_idx = p;
      match_idx = s;
      p++;
    } else if (star_idx != std::string::npos) {
      p = star_idx + 1;
      match_idx++;
      s = match_idx;
    } else {
      return false;
    }
  }
  while (p < pattern.size() && pattern[p] == '*') p++;
  return p == pattern.size();
}

void ToUpper(std::string& str) {
  std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
}

void ToLower(std::string& str) {
  std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
}
} // namespace

namespace valkey {

std::string Database::FormatInt(int64_t val) const {
  return "(integer) " + std::to_string(val);
}
std::string Database::FormatString(const std::string& val) const {
  return "\"" + val + "\"";
}
std::string Database::FormatArray(const std::vector<std::string>& arr) const {
  if (arr.empty()) return "(empty array)";
  std::string res;
  for (size_t i = 0; i < arr.size(); ++i) {
    res += std::to_string(i + 1) + ") \"" + arr[i] + "\"\n";
  }
  if (!res.empty()) res.pop_back();
  return res;
}
std::string Database::FormatError(const std::string& msg) const {
  return "(error) " + msg;
}
std::string Database::FormatNil() const { return "(nil)"; }
std::string Database::FormatOk() const { return "OK"; }

uint64_t Database::EstimateMemory(const std::string& key, const DbEntry& entry) const {
  uint64_t mem = key.size() + sizeof(DbEntry) + 32;
  mem += std::visit(
      Overloaded{
          [](const StringType& s) -> uint64_t { return s.size(); },
          [](const ListType& l) -> uint64_t {
            uint64_t s = 0;
            for (const auto& str : l) s += str.size() + 24;
            return s;
          },
          [](const SetType& st) -> uint64_t {
            uint64_t s = 0;
            for (const auto& str : st) s += str.size() + 24;
            return s;
          },
          [](const GeoType& g) -> uint64_t {
            uint64_t s = 0;
            for (const auto& [k, v] : g) s += k.size() + sizeof(GeoPoint) + 24;
            return s;
          }},
      entry.value_);
  return mem;
}

void Database::DeleteKey(const std::string& key) {
  if (auto it = storage_.find(key); it != storage_.end()) {
    current_memory_ -= EstimateMemory(key, it->second);
    storage_.erase(it);
  }
}

void Database::UpdateKey(const std::string& key, const ValueVariant& val) {
  if (auto it = storage_.find(key); it != storage_.end()) {
    current_memory_ -= EstimateMemory(key, it->second);
  }
  storage_[key].value_ = val;
  current_memory_ += EstimateMemory(key, storage_[key]);
}

bool Database::CheckMemoryLimit(uint64_t expected_additional) const {
  if (max_memory_ == 0) return true;
  return current_memory_ + expected_additional <= max_memory_;
}

bool Database::IsExpired(const std::string& key) {
  auto it = storage_.find(key);
  if (it == storage_.end()) return true;
  if (it->second.expire_at_ <= std::chrono::steady_clock::now()) {
    DeleteKey(key);
    return true;
  }
  return false;
}

void Database::CheckExpirations() {
  std::vector<std::string> to_delete;
  auto now = std::chrono::steady_clock::now();
  for (const auto& [k, v] : storage_) {
    if (v.expire_at_ <= now) {
      to_delete.push_back(k);
    }
  }
  for (const auto& k : to_delete) DeleteKey(k);
}

std::string Database::ProcessCommand(const std::vector<std::string>& args) {
  if (args.empty()) return "";
  try {
    std::string cmd = args[0];
    ToUpper(cmd);

    CheckExpirations();

    if (cmd == "SET" || cmd == "GET" || cmd == "STRLEN" || cmd == "APPEND" ||
        cmd == "EXPIRE" || cmd == "TTL") {
      return ProcessString(cmd, args);
    }
    if (cmd == "LPUSH" || cmd == "RPUSH" || cmd == "LPOP" || cmd == "RPOP" ||
        cmd == "LLEN" || cmd == "LRANGE" || cmd == "LINDEX" || cmd == "LSET" ||
        cmd == "LINSERT") {
      return ProcessList(cmd, args);
    }
    if (cmd == "SADD" || cmd == "SISMEMBER" || cmd == "SMEMBERS" || 
        cmd == "SCARD" || cmd == "SREM" || cmd == "SUNION" ||
        cmd == "SINTER" || cmd == "SDIFF" || cmd == "SMOVE") {
      return ProcessSet(cmd, args);
    }
    if (cmd == "GEOADD" || cmd == "GEODIST" || cmd == "GEOPOS" ||
        cmd == "GEOSEARCH" || cmd == "GEOSEARCHSTORE") {
      return ProcessGeo(cmd, args);
    }
    if (cmd == "TYPE" || cmd == "DEL" || cmd == "EXISTS" ||
        cmd == "FLUSHDB" || cmd == "CONFIG" || cmd == "DBSIZE" ||
        cmd == "MEMORY" || cmd == "KEYS") {
      return ProcessGeneral(cmd, args);
    }

    return FormatError("ERR unknown command '" + cmd + "'");
  } catch (const std::exception&) {
    return FormatError("ERR invalid argument or value out of range");
  }
}

std::string Database::ProcessString(const std::string& cmd, const std::vector<std::string>& args) {
  if (cmd == "SET") {
    if (args.size() != 3) return FormatError("ERR wrong number of arguments");
    if (!CheckMemoryLimit(args[1].size() + args[2].size())) {
      return FormatError("OOM command not allowed when used memory > 'maxmemory'");
    }
    UpdateKey(args[1], StringType(args[2]));
    storage_[args[1]].expire_at_ = std::chrono::steady_clock::time_point::max();
    return FormatOk();
  }
  if (cmd == "GET") {
    if (args.size() != 2) return FormatError("ERR wrong number of arguments");
    if (IsExpired(args[1])) return FormatNil();
    if (auto* str = std::get_if<StringType>(&storage_[args[1]].value_)) {
      return FormatString(*str);
    }
    return FormatError("WRONGTYPE Operation against a key holding the wrong kind of value");
  }
  if (cmd == "STRLEN") {
    if (args.size() != 2) return FormatError("ERR wrong number of arguments");
    if (IsExpired(args[1])) return FormatInt(0);
    if (auto* str = std::get_if<StringType>(&storage_[args[1]].value_)) {
      return FormatInt(str->size());
    }
    return FormatError("WRONGTYPE");
  }
  if (cmd == "APPEND") {
    if (args.size() != 3) return FormatError("ERR wrong number of arguments");
    if (!CheckMemoryLimit(args[2].size())) return FormatError("OOM command not allowed when used memory > 'maxmemory'");
    if (IsExpired(args[1])) {
      UpdateKey(args[1], StringType(args[2]));
      return FormatInt(args[2].size());
    }
    if (auto* str = std::get_if<StringType>(&storage_[args[1]].value_)) {
      std::string new_str = *str + args[2];
      UpdateKey(args[1], new_str);
      return FormatInt(new_str.size());
    }
    return FormatError("WRONGTYPE");
  }
  if (cmd == "EXPIRE") {
    if (args.size() != 3) return FormatError("ERR wrong number of arguments");
    if (IsExpired(args[1])) return FormatInt(0);
    int seconds = std::stoi(args[2]);
    storage_[args[1]].expire_at_ = std::chrono::steady_clock::now() + std::chrono::seconds(seconds);
    return FormatInt(1);
  }
  if (cmd == "TTL") {
    if (args.size() != 2) return FormatError("ERR wrong number of arguments");
    if (IsExpired(args[1])) return FormatInt(-2);
    auto expire_at = storage_[args[1]].expire_at_;
    if (expire_at == std::chrono::steady_clock::time_point::max()) return FormatInt(-1);
    return FormatInt(std::chrono::duration_cast<std::chrono::seconds>(expire_at - std::chrono::steady_clock::now()).count());
  }
  return FormatError("ERR unknown string command");
}

std::string Database::ProcessList(const std::string& cmd, const std::vector<std::string>& args) {
  if (args.size() < 2) return FormatError("ERR wrong number of arguments");
  const std::string& key = args[1];
  if (!IsExpired(key) && !std::holds_alternative<ListType>(storage_[key].value_)) return FormatError("WRONGTYPE");

  if (cmd == "LPUSH" || cmd == "RPUSH") {
    if (args.size() < 3) return FormatError("ERR wrong number of arguments");
    if (!CheckMemoryLimit(args.size() * 32)) return FormatError("OOM command not allowed when used memory > 'maxmemory'");
    ListType lst;
    if (!IsExpired(key)) lst = std::get<ListType>(storage_[key].value_);
    for (size_t i = 2; i < args.size(); ++i) {
      if (cmd == "LPUSH") lst.push_front(args[i]);
      else lst.push_back(args[i]);
    }
    size_t sz = lst.size();
    UpdateKey(key, lst);
    return FormatInt(sz);
  }
  if (cmd == "LPOP" || cmd == "RPOP") {
    if (IsExpired(key)) return FormatNil();
    auto& lst = std::get<ListType>(storage_[key].value_);
    int count = 1;
    bool has_count = false;
    if (args.size() == 3) {
      count = std::stoi(args[2]);
      has_count = true;
    }
    std::vector<std::string> res;
    while (count > 0 && !lst.empty()) {
      if (cmd == "LPOP") { res.push_back(lst.front()); lst.pop_front(); }
      else { res.push_back(lst.back()); lst.pop_back(); }
      count--;
    }
    UpdateKey(key, lst);
    if (lst.empty()) DeleteKey(key);
    if (res.empty()) return FormatNil();
    if (has_count) return FormatArray(res);
    return FormatString(res[0]);
  }
  if (cmd == "LLEN") {
    if (args.size() != 2) return FormatError("ERR wrong number of arguments");
    if (IsExpired(key)) return FormatInt(0);
    return FormatInt(std::get<ListType>(storage_[key].value_).size());
  }
  if (cmd == "LRANGE") {
    if (args.size() != 4) return FormatError("ERR wrong number of arguments");
    if (IsExpired(key)) return FormatArray({});
    const auto& lst = std::get<ListType>(storage_[key].value_);
    int start = std::stoi(args[2]);
    int stop = std::stoi(args[3]);
    int size = static_cast<int>(lst.size());
    if (start < 0) start += size;
    if (stop < 0) stop += size;
    if (start < 0) start = 0;
    if (stop >= size) stop = size - 1;
    if (start > stop || start >= size) return FormatArray({});
    auto it = lst.begin();
    std::advance(it, start);
    std::vector<std::string> res;
    for (int i = start; i <= stop; ++i) {
      res.push_back(*it);
      ++it;
    }
    return FormatArray(res);
  }
  if (cmd == "LINDEX") {
    if (args.size() != 3) return FormatError("ERR wrong number of arguments");
    if (IsExpired(key)) return FormatNil();
    const auto& lst = std::get<ListType>(storage_[key].value_);
    int index = std::stoi(args[2]);
    int size = static_cast<int>(lst.size());
    if (index < 0) index += size;
    if (index < 0 || index >= size) return FormatNil();
    auto it = lst.begin();
    std::advance(it, index);
    return FormatString(*it);
  }
  if (cmd == "LSET") {
    if (args.size() != 4) return FormatError("ERR wrong number of arguments");
    if (IsExpired(key)) return FormatError("ERR no such key");
    auto lst = std::get<ListType>(storage_[key].value_);
    int index = std::stoi(args[2]);
    int size = static_cast<int>(lst.size());
    if (index < 0) index += size;
    if (index < 0 || index >= size) return FormatError("ERR index out of range");
    auto it = lst.begin();
    std::advance(it, index);
    if (!CheckMemoryLimit(args[3].size())) return FormatError("OOM command not allowed when used memory > 'maxmemory'");
    *it = args[3];
    UpdateKey(key, lst);
    return FormatOk();
  }
  if (cmd == "LINSERT") {
    if (args.size() != 5) return FormatError("ERR wrong number of arguments");
    if (IsExpired(key)) return FormatInt(0);
    auto lst = std::get<ListType>(storage_[key].value_);
    std::string where = args[2];
    ToUpper(where);
    if (where != "BEFORE" && where != "AFTER") return FormatError("ERR syntax error");
    std::string pivot = args[3];
    std::string value = args[4];
    auto it = std::find(lst.begin(), lst.end(), pivot);
    if (it == lst.end()) return FormatInt(-1);
    if (where == "AFTER") ++it;
    if (!CheckMemoryLimit(value.size() + 32)) return FormatError("OOM command not allowed when used memory > 'maxmemory'");
    lst.insert(it, value);
    int sz = static_cast<int>(lst.size());
    UpdateKey(key, lst);
    return FormatInt(sz);
  }
  return FormatError("ERR list command not fully implemented");
}

std::string Database::ProcessSet(const std::string& cmd, const std::vector<std::string>& args) {
  if (args.size() < 2) return FormatError("ERR wrong number of arguments");

  if (cmd == "SUNION" || cmd == "SINTER" || cmd == "SDIFF") {
    std::vector<SetType> sets;
    for (size_t i = 1; i < args.size(); ++i) {
      if (IsExpired(args[i])) {
        sets.push_back({});
      } else {
        if (!std::holds_alternative<SetType>(storage_[args[i]].value_)) return FormatError("WRONGTYPE");
        sets.push_back(std::get<SetType>(storage_[args[i]].value_));
      }
    }
    SetType res;
    if (cmd == "SUNION") {
      for (const auto& s : sets) {
        for (const auto& elem : s) res.insert(elem);
      }
    } else if (cmd == "SINTER") {
      if (!sets.empty()) {
        res = sets[0];
        for (size_t i = 1; i < sets.size(); ++i) {
          SetType next_res;
          for (const auto& elem : res) {
            if (sets[i].count(elem)) next_res.insert(elem);
          }
          res = next_res;
        }
      }
    } else if (cmd == "SDIFF") {
      if (!sets.empty()) {
        res = sets[0];
        for (size_t i = 1; i < sets.size(); ++i) {
          for (const auto& elem : sets[i]) res.erase(elem);
        }
      }
    }
    std::vector<std::string> vec(res.begin(), res.end());
    return FormatArray(vec);
  }

  if (cmd == "SMOVE") {
    if (args.size() != 4) return FormatError("ERR wrong number of arguments");
    std::string src = args[1];
    std::string dst = args[2];
    std::string member = args[3];
    if (IsExpired(src)) return FormatInt(0);
    if (!std::holds_alternative<SetType>(storage_[src].value_)) return FormatError("WRONGTYPE");
    auto src_st = std::get<SetType>(storage_[src].value_);
    if (!src_st.count(member)) return FormatInt(0);
    if (!IsExpired(dst) && !std::holds_alternative<SetType>(storage_[dst].value_)) return FormatError("WRONGTYPE");
    
    src_st.erase(member);
    UpdateKey(src, src_st);
    if (src_st.empty()) DeleteKey(src);

    SetType dst_st;
    if (!IsExpired(dst)) dst_st = std::get<SetType>(storage_[dst].value_);
    if (!CheckMemoryLimit(member.size() + 32)) return FormatError("OOM command not allowed when used memory > 'maxmemory'");
    dst_st.insert(member);
    UpdateKey(dst, dst_st);
    return FormatInt(1);
  }

  const std::string& key = args[1];
  if (!IsExpired(key) && !std::holds_alternative<SetType>(storage_[key].value_)) return FormatError("WRONGTYPE");

  if (cmd == "SADD") {
    if (args.size() < 3) return FormatError("ERR wrong number of arguments");

    uint64_t expected_additional = 0;
    for (size_t i = 2; i < args.size(); ++i) expected_additional += args[i].size() + 24;
    if (!CheckMemoryLimit(expected_additional)) return FormatError("OOM command not allowed when used memory > 'maxmemory'");

    if (!storage_.count(key)) {
      storage_[key].value_ = SetType{};
    } else {
      current_memory_ -= EstimateMemory(key, storage_[key]);
    }

    auto& st = std::get<SetType>(storage_[key].value_);
    int added = 0;
    for (size_t i = 2; i < args.size(); ++i) {
      if (st.insert(args[i]).second) added++;
    }
    current_memory_ += EstimateMemory(key, storage_[key]);
    return FormatInt(added);
  }
  if (cmd == "SCARD") {
    if (args.size() != 2) return FormatError("ERR wrong number of arguments");
    if (IsExpired(key)) return FormatInt(0);
    return FormatInt(std::get<SetType>(storage_[key].value_).size());
  }
  if (cmd == "SREM") {
    if (args.size() < 3) return FormatError("ERR wrong number of arguments");
    if (IsExpired(key)) return FormatInt(0);
    auto st = std::get<SetType>(storage_[key].value_);
    int removed = 0;
    for (size_t i = 2; i < args.size(); ++i) {
      if (st.erase(args[i])) removed++;
    }
    UpdateKey(key, st);
    if (st.empty()) DeleteKey(key);
    return FormatInt(removed);
  }
  if (cmd == "SISMEMBER") {
    if (args.size() != 3) return FormatError("ERR wrong number of arguments");
    if (IsExpired(key)) return FormatInt(0);
    const auto& st = std::get<SetType>(storage_[key].value_);
    return FormatInt(st.count(args[2]));
  }
  if (cmd == "SMEMBERS") {
    if (args.size() != 2) return FormatError("ERR wrong number of arguments");
    if (IsExpired(key)) return FormatArray({});
    const auto& st = std::get<SetType>(storage_[key].value_);
    std::vector<std::string> res(st.begin(), st.end());
    return FormatArray(res);
  }

  return FormatError("ERR set command not fully implemented");
}

std::string Database::ProcessGeo(const std::string& cmd, const std::vector<std::string>& args) {
  if (args.size() < 2) return FormatError("ERR wrong number of arguments");

  if (cmd == "GEOSEARCH" || cmd == "GEOSEARCHSTORE") {
    std::string source_key, dest_key;
    size_t arg_idx = 1;
    if (cmd == "GEOSEARCHSTORE") {
      if (args.size() < 8) return FormatError("ERR wrong number of arguments");
      dest_key = args[arg_idx++];
      source_key = args[arg_idx++];
    } else {
      if (args.size() < 7) return FormatError("ERR wrong number of arguments");
      source_key = args[arg_idx++];
    }
    
    if (IsExpired(source_key)) return cmd == "GEOSEARCH" ? FormatArray({}) : FormatInt(0);
    if (!std::holds_alternative<GeoType>(storage_[source_key].value_)) return FormatError("WRONGTYPE");
    const auto& gt = std::get<GeoType>(storage_[source_key].value_);
    
    double lon = 0, lat = 0;
    std::string from = args[arg_idx++];
    ToUpper(from);
    if (from != "FROMLONLAT") return FormatError("ERR unsupported FROM option");
    lon = std::stod(args[arg_idx++]);
    lat = std::stod(args[arg_idx++]);
    
    std::string by = args[arg_idx++];
    ToUpper(by);
    if (by != "BYRADIUS") return FormatError("ERR unsupported BY option");
    double radius = std::stod(args[arg_idx++]);
    std::string unit = args[arg_idx++];
    
    bool asc = true;
    bool sort_specified = false;
    int count = -1;
    
    while (arg_idx < args.size()) {
      std::string opt = args[arg_idx++];
      ToUpper(opt);
      if (opt == "ASC") { asc = true; sort_specified = true; }
      else if (opt == "DESC") { asc = false; sort_specified = true; }
      else if (opt == "COUNT" && arg_idx < args.size()) {
        count = std::stoi(args[arg_idx++]);
      }
    }
    
    GeoPoint center{lon, lat};
    std::vector<std::pair<std::string, double>> results;
    for (const auto& [member, p] : gt) {
      double dist = CalculateDistance(center, p);
      double dist_in_unit = ConvertMetersToUnit(dist, unit);
      if (dist_in_unit <= radius) {
        results.push_back({member, dist});
      }
    }
    
    if (sort_specified) {
      if (asc) {
        std::sort(results.begin(), results.end(), [](const auto& a, const auto& b){ return a.second < b.second; });
      } else {
        std::sort(results.begin(), results.end(), [](const auto& a, const auto& b){ return a.second > b.second; });
      }
    }
    
    if (count > 0 && results.size() > (size_t)count) {
      results.resize(count);
    }
    
    if (cmd == "GEOSEARCH") {
      std::vector<std::string> res;
      for (const auto& r : results) res.push_back(r.first);
      return FormatArray(res);
    } else {
      GeoType dest_gt;
      for (const auto& r : results) {
        dest_gt[r.first] = gt.at(r.first);
      }
      uint64_t new_mem = dest_key.size() + sizeof(DbEntry) + 32;
      for (const auto& [k, v] : dest_gt) new_mem += k.size() + sizeof(GeoPoint) + 24;
      uint64_t old_mem = 0;
      if (storage_.count(dest_key)) old_mem = EstimateMemory(dest_key, storage_[dest_key]);
      if (!CheckMemoryLimit(new_mem > old_mem ? new_mem - old_mem : 0)) {
         return FormatError("OOM command not allowed when used memory > 'maxmemory'");
      }
      UpdateKey(dest_key, dest_gt);
      storage_[dest_key].expire_at_ = std::chrono::steady_clock::time_point::max();
      return FormatInt(results.size());
    }
  }

  const std::string& key = args[1];
  if (!IsExpired(key) && !std::holds_alternative<GeoType>(storage_[key].value_)) return FormatError("WRONGTYPE");

  if (cmd == "GEOADD") {
    if (args.size() < 5 || (args.size() - 2) % 3 != 0) return FormatError("ERR wrong number of arguments");
    GeoType gt;
    if (!IsExpired(key)) gt = std::get<GeoType>(storage_[key].value_);
    int added = 0;
    for (size_t i = 2; i + 2 < args.size(); i += 3) {
      if (!CheckMemoryLimit(32)) return FormatError("OOM command not allowed when used memory > 'maxmemory'");
      GeoPoint p{std::stod(args[i]), std::stod(args[i+1])};
      if (gt.find(args[i+2]) == gt.end()) added++;
      gt[args[i+2]] = p;
    }
    UpdateKey(key, gt);
    return FormatInt(added);
  }
  if (cmd == "GEODIST") {
    if (args.size() < 4 || args.size() > 5) return FormatError("ERR wrong number of arguments");
    if (IsExpired(key)) return FormatNil();
    auto& gt = std::get<GeoType>(storage_[key].value_);
    if (gt.count(args[2]) == 0 || gt.count(args[3]) == 0) return FormatNil();
    std::string unit = args.size() == 5 ? args[4] : "m";
    return FormatString(std::to_string(ConvertMetersToUnit(CalculateDistance(gt[args[2]], gt[args[3]]), unit)));
  }
  if (cmd == "GEOPOS") {
    if (args.size() < 3) return FormatError("ERR wrong number of arguments");
    if (IsExpired(key)) return FormatArray({});
    const auto& gt = std::get<GeoType>(storage_[key].value_);
    std::string res;
    for (size_t i = 2; i < args.size(); ++i) {
      res += std::to_string(i - 1) + ") ";
      if (gt.count(args[i])) {
        const auto& p = gt.at(args[i]);
        res += "1) \"" + std::to_string(p.longitude_) + "\"\n   2) \"" + std::to_string(p.latitude_) + "\"\n";
      } else {
        res += "(nil)\n";
      }
    }
    if (!res.empty()) res.pop_back();
    return res;
  }
  return FormatError("ERR geo command not fully implemented");
}

std::string Database::ProcessGeneral(const std::string& cmd, const std::vector<std::string>& args) {
  if (cmd == "DEL") {
    if (args.size() < 2) return FormatError("ERR wrong number of arguments");
    int deleted = 0;
    for (size_t i = 1; i < args.size(); ++i) {
      if (!IsExpired(args[i])) { DeleteKey(args[i]); deleted++; }
    }
    return FormatInt(deleted);
  }
  if (cmd == "EXISTS") {
    if (args.size() < 2) return FormatError("ERR wrong number of arguments");
    int exists = 0;
    for (size_t i = 1; i < args.size(); ++i) {
      if (!IsExpired(args[i])) exists++;
    }
    return FormatInt(exists);
  }
  if (cmd == "TYPE") {
    if (args.size() != 2) return FormatError("ERR wrong number of arguments");
    if (IsExpired(args[1])) return "none";
    return std::visit(Overloaded{
            [](const StringType&) { return "string"; },
            [](const ListType&) { return "list"; },
            [](const SetType&) { return "set"; },
            [](const GeoType&) { return "zset"; }}, storage_[args[1]].value_);
  }
  if (cmd == "CONFIG") {
    if (args.size() >= 3) {
      std::string sub = args[1];
      ToUpper(sub);
      std::string param = args[2];
      ToLower(param);
      
      if (args.size() == 3 && sub == "GET" && param == "maxmemory") {
        return FormatArray({"maxmemory", std::to_string(max_memory_)});
      }
      if (args.size() == 4 && sub == "SET" && param == "maxmemory") {
        std::string s = args[3];
        ToLower(s);
        uint64_t mult = 1;
        if (s.ends_with("kb")) { mult = 1024; s.resize(s.size() - 2); }
        else if (s.ends_with("mb")) { mult = 1024 * 1024; s.resize(s.size() - 2); }
        else if (s.ends_with("gb")) { mult = 1024 * 1024 * 1024; s.resize(s.size() - 2); }
        else if (s.ends_with("k")) { mult = 1024; s.resize(s.size() - 1); }
        else if (s.ends_with("m")) { mult = 1024 * 1024; s.resize(s.size() - 1); }
        else if (s.ends_with("g")) { mult = 1024 * 1024 * 1024; s.resize(s.size() - 1); }
        else if (s.ends_with("b")) { s.resize(s.size() - 1); }
        try {
          max_memory_ = std::stoull(s) * mult;
        } catch (...) {
          return FormatError("ERR invalid value for maxmemory");
        }
        return FormatOk();
      }
    }
    return FormatError("ERR unsupported CONFIG option");
  }
  if (cmd == "DBSIZE") {
    if (args.size() != 1) return FormatError("ERR wrong number of arguments");
    return FormatInt(storage_.size());
  }
  if (cmd == "KEYS") {
    if (args.size() != 2) return FormatError("ERR wrong number of arguments");
    std::vector<std::string> res;
    std::vector<std::string> to_delete;
    auto now = std::chrono::steady_clock::now();
    for (const auto& [k, v] : storage_) {
      if (Match(args[1], k)) {
        if (v.expire_at_ <= now) to_delete.push_back(k);
        else res.push_back(k);
      }
    }
    for (const auto& k : to_delete) DeleteKey(k);
    return FormatArray(res);
  }
  if (cmd == "FLUSHDB") {
    if (args.size() != 1) return FormatError("ERR wrong number of arguments");
    storage_.clear();
    current_memory_ = 0;
    return FormatOk();
  }
  if (cmd == "MEMORY") {
    if (args.size() == 3) {
      std::string sub = args[1];
      ToUpper(sub);
      if (sub == "USAGE") {
        if (IsExpired(args[2])) return FormatNil();
        return FormatInt(EstimateMemory(args[2], storage_[args[2]]));
      }
    }
    return FormatError("ERR unsupported MEMORY option");
  }
  return FormatError("ERR general command not fully implemented");
}

} // namespace valkey