#pragma once

#include <utility>
#include <iterator>
#include <concepts>

namespace detail {
    using std::begin;
    using std::end;

    template <typename T>
    concept RangeConcept = requires(T& t) {
        begin(t);
        end(t);
    };
}

template <typename T>
concept IsRange = detail::RangeConcept<T>;

template <typename Derived>
struct Pipeable {
    Pipeable() = default;
};

template <IsRange Range, typename Derived>
decltype(auto) operator|(Range&& range, const Pipeable<Derived>& adapter) {
    return static_cast<const Derived&>(adapter)(std::forward<Range>(range));
}
