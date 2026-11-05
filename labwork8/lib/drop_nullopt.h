#pragma once

#include "pipe.h"
#include <iterator>
#include <optional>
#include <utility>
#include <type_traits>

template <typename Iterator, typename Sentinel>
class DropNulloptIterator {
public:
    using iterator_category = std::input_iterator_tag;
    using base_reference = typename std::iterator_traits<Iterator>::reference;
    using optional_ref = decltype(**std::declval<Iterator&>());
    using value_type = std::remove_cvref_t<optional_ref>;
    using difference_type = typename std::iterator_traits<Iterator>::difference_type;
    using reference = std::conditional_t<std::is_reference_v<base_reference>, optional_ref, value_type>;
    using pointer = value_type*;

    DropNulloptIterator() = default;

    DropNulloptIterator(Iterator it, Sentinel end)
        : it_(std::move(it)), end_(std::move(end)) {
        SkipNullopts();
    }

    reference operator*() {
        return **it_;
    }

    DropNulloptIterator& operator++() {
        ++it_;
        SkipNullopts();
        return *this;
    }

    bool operator==(const DropNulloptIterator& other) const {
        return it_ == other.it_;
    }

    bool operator!=(const DropNulloptIterator& other) const {
        return !(*this == other);
    }

private:
    Iterator it_;
    Sentinel end_;

    void SkipNullopts() {
        while (it_ != end_ && !(*it_).has_value()) {
            ++it_;
        }
    }
};

template <IsRange Range>
class DropNulloptView {
public:
    explicit DropNulloptView(Range range) : range_(std::move(range)) {}

    // NOLINTNEXTLINE
    auto begin() { return DropNulloptIterator(std::begin(range_), std::end(range_)); }
    // NOLINTNEXTLINE
    auto end() { return DropNulloptIterator(std::end(range_), std::end(range_)); }

private:
    Range range_;
};

struct DropNulloptClosure : Pipeable<DropNulloptClosure> {
    template <IsRange Range>
    auto operator()(Range&& range) const {
        return DropNulloptView<std::decay_t<Range>>(std::forward<Range>(range));
    }
};

inline auto DropNullopt() {
    return DropNulloptClosure{};
}