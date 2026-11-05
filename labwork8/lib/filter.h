#pragma once

#include "pipe.h"
#include <iterator>
#include <utility>
#include <type_traits>
#include <functional>

template <typename Iterator, typename Sentinel, typename Predicate>
class FilterIterator {
public:
    using iterator_category = std::input_iterator_tag;
    using value_type = typename std::iterator_traits<Iterator>::value_type;
    using difference_type = typename std::iterator_traits<Iterator>::difference_type;
    using pointer = typename std::iterator_traits<Iterator>::pointer;
    using reference = typename std::iterator_traits<Iterator>::reference;

    FilterIterator() = default;

    FilterIterator(Iterator it, Sentinel end, Predicate pred)
        : it_(std::move(it)), end_(std::move(end)), pred_(std::move(pred)) {

        SatisfyPredicate();
    }

    reference operator*() { return *it_; }
    pointer operator->() { return &(*it_); }

    FilterIterator& operator++() {
        ++it_;
        SatisfyPredicate();
        return *this;
    }

    bool operator==(const FilterIterator& other) const { return it_ == other.it_; }
    bool operator!=(const FilterIterator& other) const { return it_ != other.it_; }

private:
    Iterator it_;
    Sentinel end_;
    Predicate pred_;

    void SatisfyPredicate() {
        while (it_ != end_ && !std::invoke(pred_, *it_)) {
            ++it_;
        }
    }
};

template <IsRange Range, typename Predicate>
class FilterView {
public:
    FilterView(Range range, Predicate pred)
        : range_(std::move(range)), pred_(std::move(pred)) {}

    // NOLINTNEXTLINE
    auto begin() {
        return FilterIterator(std::begin(range_), std::end(range_), pred_);
    }

    // NOLINTNEXTLINE
    auto end() {
        return FilterIterator(std::end(range_), std::end(range_), pred_);
    }

private:
    Range range_;
    Predicate pred_;
};

template <typename Predicate>
struct FilterClosure : Pipeable<FilterClosure<Predicate>> {
    Predicate pred_;

    explicit FilterClosure(Predicate pred) : pred_(std::move(pred)) {}

    template <IsRange Range>
    auto operator()(Range&& range) const {
        return FilterView<std::decay_t<Range>, Predicate>(std::forward<Range>(range), pred_);
    }
};

template <typename Predicate>
auto Filter(Predicate pred) {
    return FilterClosure<Predicate>(std::move(pred));
}