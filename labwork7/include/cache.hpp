#pragma once

#include <chrono>
#include <unordered_map>
#include <list>
#include <optional>
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <nlohmann/json.hpp> // NOLINT(misc-include-cleaner)
#include "models.hpp"
// NOLINTBEGIN(readability-identifier-naming) 
namespace nlohmann {
    template <typename Clock, typename Duration>
    struct adl_serializer<std::chrono::time_point<Clock, Duration>> {
        static void to_json(json& j, const std::chrono::time_point<Clock, Duration>& tp) {
            auto duration = tp.time_since_epoch();
            j = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
        }

        static void from_json(const json& j, std::chrono::time_point<Clock, Duration>& tp) {
            auto milliseconds = j.get<std::chrono::milliseconds::rep>();
            tp = std::chrono::time_point<Clock, Duration>(std::chrono::milliseconds(milliseconds));
        }
    };
} // nlohmann
// NOLINTEND(readability-identifier-naming)
using json = nlohmann::json;

template<typename Key, typename Value>
class PersistentTimedLRUCache {
private:
    struct CacheEntry {
        Value value;
        std::chrono::system_clock::time_point created_at;
        std::chrono::system_clock::time_point expiry;
        typename std::list<Key>::iterator lru_it;
        
        NLOHMANN_DEFINE_TYPE_INTRUSIVE(CacheEntry, value, created_at, expiry)
    };

    size_t capacity_;
    std::chrono::milliseconds max_age_;
    std::unordered_map<Key, CacheEntry> cache_;
    std::list<Key> lru_list_;
    std::string filename_;

    static std::chrono::system_clock::time_point TimeFromString(const std::string& tp_str) {
        return std::chrono::system_clock::time_point(
            std::chrono::milliseconds(std::stoll(tp_str)));
    }

    static std::string TimeToString(std::chrono::system_clock::time_point tp) {
        return std::to_string(
            std::chrono::duration_cast<std::chrono::milliseconds>(
                tp.time_since_epoch()).count());
    }

    void EvictIfExpired(const Key& key) {
        auto it = cache_.find(key);
        if (it != cache_.end()) {
            auto now = std::chrono::system_clock::now();
            if (now >= it->second.expiry) {
                lru_list_.erase(it->second.lru_it);
                cache_.erase(it);
            }
        }
    }

    void EvictOldest() {
        while (cache_.size() >= capacity_) {
            Key oldest = lru_list_.back();
            lru_list_.pop_back();
            cache_.erase(oldest);
        }
    }

    void LoadFromFile() {
        try {
            std::ifstream file(filename_);
            if (!file.is_open()) {
                return;
            }
            
            json j;
            file >> j;
            
            auto now = std::chrono::system_clock::now();
            
            if (j.contains("metadata") && j["metadata"].contains("timestamp")) {
                auto save_time = TimeFromString(j["metadata"]["timestamp"]);
                if (now - save_time > max_age_) {
                    return;
                }
            }
            
            if (j.contains("entries")) {
                for (auto& [key, entry_json] : j["entries"].items()) {
                    CacheEntry entry;
                    entry.value = entry_json["value"].get<Value>();
                    entry.created_at = TimeFromString(entry_json["created_at"]);
                    entry.expiry = TimeFromString(entry_json["expiry"]);
                    
                    if (now < entry.expiry) {
                        cache_[key] = entry;
                    }
                }
            }
            
            if (j.contains("lru_order")) {
                for (const auto& key : j["lru_order"]) {
                    auto it = cache_.find(key);
                    if (it != cache_.end()) {
                        lru_list_.push_back(key);
                        it->second.lru_it = std::prev(lru_list_.end());
                    }
                }
            }
            
        } catch (const std::exception& e) {
            cache_.clear();
            lru_list_.clear();
        }
    }

    void SaveToFile() const {
        try {
            json j;
            
            j["metadata"] = {
                {"capacity", capacity_},
                {"max_age_ms", max_age_.count()},
                {"timestamp", TimeToString(std::chrono::system_clock::now())}
            };
            
            json entries = json::object();
            for (const auto& [key, entry] : cache_) {
                json entry_json;
                entry_json["value"] = entry.value;
                entry_json["created_at"] = TimeToString(entry.created_at);
                entry_json["expiry"] = TimeToString(entry.expiry);
                entries[key] = entry_json;
            }
            j["entries"] = entries;
            
            json lru_order = json::array();
            for (const auto& key : lru_list_) {
                lru_order.push_back(key);
            }
            j["lru_order"] = lru_order;
            
            std::ofstream file(filename_);
            if (file.is_open()) {
                file << j.dump(2);
            }
        } catch (const std::exception& e) {
            // Silently ignore save errors
        }
    }

public:
    explicit PersistentTimedLRUCache(size_t capacity, 
                                     std::chrono::milliseconds max_age,
                                     const std::string& filename = "cache.json")
        : capacity_(capacity), max_age_(max_age), filename_(filename) {
        LoadFromFile();
    }

    ~PersistentTimedLRUCache() {
        SaveToFile();
    }

    void Put(const Key& key, const Value& value) {
        auto it = cache_.find(key);
        if (it != cache_.end()) {
            lru_list_.erase(it->second.lru_it);
            cache_.erase(it);
        }
        
        EvictOldest();
        
        auto now = std::chrono::system_clock::now();
        lru_list_.push_front(key);
        cache_[key] = {
            value,
            now,
            now + max_age_,
            lru_list_.begin()
        };
        
        SaveToFile();
    }

    std::optional<Value> Get(const Key& key) {
        EvictIfExpired(key);
        
        auto it = cache_.find(key);
        if (it == cache_.end()) {
            return std::nullopt;
        }
        
        lru_list_.erase(it->second.lru_it);
        lru_list_.push_front(key);
        it->second.lru_it = lru_list_.begin();
        
        return it->second.value;
    }

    bool Contains(const Key& key) const {
        auto it = cache_.find(key);
        if (it == cache_.end()) return false;
        
        auto now = std::chrono::system_clock::now();
        return now < it->second.expiry;
    }

    void Clear() {
        cache_.clear();
        lru_list_.clear();
        SaveToFile();
    }

    size_t Size() const {
        return cache_.size();
    }

    std::vector<Key> GetAllKeys() const {
        std::vector<Key> keys;
        for (const auto& [key, _] : cache_) {
            keys.push_back(key);
        }
        return keys;
    }

    std::optional<std::pair<std::chrono::system_clock::time_point, 
                           std::chrono::system_clock::time_point>> 
    GetEntryTimes(const Key& key) const {
        auto it = cache_.find(key);
        if (it == cache_.end()) return std::nullopt;
        return std::make_pair(it->second.created_at, it->second.expiry);
    }
};
