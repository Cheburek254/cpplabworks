#pragma once

#include "pipe.h"
#include <iterator>
#include <utility>
#include <type_traits>

template <typename Iterator, typename Sentinel>
class ExpectedValuesIterator {
public:
    using iterator_category = std::input_iterator_tag;
    using base_reference = typename std::iterator_traits<Iterator>::reference;
    using expected_ref = decltype(**std::declval<Iterator&>());
    using value_type = std::remove_cvref_t<expected_ref>;
    using difference_type = typename std::iterator_traits<Iterator>::difference_type;
    using reference = std::conditional_t<std::is_reference_v<base_reference>, expected_ref, value_type>;
    using pointer = value_type*;

    ExpectedValuesIterator() = default;

    ExpectedValuesIterator(Iterator it, Sentinel end)
        : it_(std::move(it)), end_(std::move(end)) {
        SkipErrors();
    }

    reference operator*() { return **it_; }

    ExpectedValuesIterator& operator++() {
        ++it_;
        SkipErrors();
        return *this;
    }

    bool operator==(const ExpectedValuesIterator& other) const { return it_ == other.it_; }
    bool operator!=(const ExpectedValuesIterator& other) const { return it_ != other.it_; }

private:
    Iterator it_;
    Sentinel end_;

    void SkipErrors() {
        while (it_ != end_ && !(*it_).has_value()) {
            ++it_;
        }
    }
};


template <typename Iterator, typename Sentinel>
class ExpectedErrorsIterator {
public:
    using iterator_category = std::input_iterator_tag;
    using base_reference = typename std::iterator_traits<Iterator>::reference;
    using expected_ref = decltype((*std::declval<Iterator&>()).error());
    using value_type = std::remove_cvref_t<expected_ref>;
    using difference_type = typename std::iterator_traits<Iterator>::difference_type;
    using reference = std::conditional_t<std::is_reference_v<base_reference>, expected_ref, value_type>;
    using pointer = value_type*;

    ExpectedErrorsIterator() = default;

    ExpectedErrorsIterator(Iterator it, Sentinel end)
        : it_(std::move(it)), end_(std::move(end)) {
        SkipValues();
    }

    reference operator*() { return (*it_).error(); }

    ExpectedErrorsIterator& operator++() {
        ++it_;
        SkipValues();
        return *this;
    }

    bool operator==(const ExpectedErrorsIterator& other) const { return it_ == other.it_; }
    bool operator!=(const ExpectedErrorsIterator& other) const { return it_ != other.it_; }

private:
    Iterator it_;
    Sentinel end_;

    void SkipValues() {
        while (it_ != end_ && (*it_).has_value()) {
            ++it_;
        }
    }
};

template <IsRange Range>
struct ExpectedValuesView {
    Range range_;
    // NOLINTNEXTLINE
    auto begin() { return ExpectedValuesIterator(std::begin(range_), std::end(range_)); }
    // NOLINTNEXTLINE
    auto end() { return ExpectedValuesIterator(std::end(range_), std::end(range_)); }
};

template <IsRange Range>
struct ExpectedErrorsView {
    Range range_;
    // NOLINTNEXTLINE
    auto begin() { return ExpectedErrorsIterator(std::begin(range_), std::end(range_)); }
    // NOLINTNEXTLINE
    auto end() { return ExpectedErrorsIterator(std::end(range_), std::end(range_)); }
};

template <IsRange Range>
struct SplitExpectedResult {
    ExpectedErrorsView<Range> unexpected;
    ExpectedValuesView<Range> expected;
};

struct SplitExpectedClosure : Pipeable<SplitExpectedClosure> {
    template <IsRange Range>
    auto operator()(Range&& range) const {
        using R = std::decay_t<Range>;
        return SplitExpectedResult<R>{
            ExpectedErrorsView<R>{range},
            ExpectedValuesView<R>{range}
        };
    }
};

inline auto SplitExpected() {
    return SplitExpectedClosure{};
}