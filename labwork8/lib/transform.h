#pragma once

#include "pipe.h"
#include <cstddef>
#include <iterator>
#include <utility>
#include <type_traits>
#include <functional>

template <typename Iterator, typename F>
class TransformIterator {
public:
    using iterator_category = std::input_iterator_tag;
    using reference = std::invoke_result_t<F&, typename std::iterator_traits<Iterator>::reference>;
    using value_type = std::remove_cvref_t<reference>;
    using difference_type = std::ptrdiff_t;
    using pointer = value_type*;

    TransformIterator() = default;

    TransformIterator(Iterator it, F func)
        : it_(std::move(it)), func_(std::move(func)) {}

    reference operator*() {
        return std::invoke(func_, *it_);
    }

    TransformIterator& operator++() {
        ++it_;
        return *this;
    }

    bool operator==(const TransformIterator& other) const { return it_ == other.it_; }
    bool operator!=(const TransformIterator& other) const { return it_ != other.it_; }

private:
    Iterator it_;
    F func_;
};

template <IsRange Range, typename F>
class TransformView {
public:
    TransformView(Range range, F func)
        : range_(std::move(range)), func_(std::move(func)) {}

    // NOLINTNEXTLINE
    auto begin() {
        return TransformIterator<decltype(std::begin(range_)), F>(std::begin(range_), func_);
    }

    // NOLINTNEXTLINE
    auto end() {
        return TransformIterator<decltype(std::end(range_)), F>(std::end(range_), func_);
    }

private:
    Range range_;
    F func_;
};

template <typename F>
struct TransformClosure : Pipeable<TransformClosure<F>> {
    F func_;

    explicit TransformClosure(F func) : func_(std::move(func)) {}

    template <IsRange Range>
    auto operator()(Range&& range) const {
        return TransformView<std::decay_t<Range>, F>(std::forward<Range>(range), func_);
    }
};

template <typename F>
auto Transform(F func) {
    return TransformClosure<F>(std::move(func));
}