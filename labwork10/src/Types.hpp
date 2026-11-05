#pragma once

#include <chrono>
#include <concepts>
#include <list>
#include <string>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <variant>

namespace valkey {

struct GeoPoint {
  double longitude_;
  double latitude_;
};

using StringType = std::string;
using ListType = std::list<std::string>;
using SetType = std::unordered_set<std::string>;
using GeoType = std::unordered_map<std::string, GeoPoint>;

using ValueVariant = std::variant<StringType, ListType, SetType, GeoType>;

struct DbEntry {
  ValueVariant value_;
  std::chrono::steady_clock::time_point expire_at_{
      std::chrono::steady_clock::time_point::max()};
};

template <class... Ts> struct Overloaded : Ts... { using Ts::operator()...; };
template <class... Ts> Overloaded(Ts...) -> Overloaded<Ts...>;

template <typename T, typename = void> struct HasSizeMeta : std::false_type {};
template <typename T>
struct HasSizeMeta<T, std::void_t<decltype(std::declval<T>().size())>>
    : std::true_type {};

template <typename T>
concept HasSizeConcept =
    requires(T a) { { a.size() } -> std::convertible_to<std::size_t>; };

} // namespace valkey