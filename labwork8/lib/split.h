#pragma once

#include "pipe.h"
#include <cstddef>
#include <iterator>
#include <istream>
#include <string>
#include <utility>
#include <type_traits>

template <typename RangeIterator, typename RangeSentinel>
class SplitIterator {
public:
    using iterator_category = std::input_iterator_tag;
    using value_type = std::string;
    using difference_type = std::ptrdiff_t;
    using pointer = std::string*;
    using reference = std::string&;

    SplitIterator() : at_end_(true) {}

    SplitIterator(RangeIterator it, RangeSentinel end, std::string delimiters)
        : it_(std::move(it)), end_(std::move(end)), delimiters_(std::move(delimiters)), at_end_(false) {
        if (it_ == end_) {
            at_end_ = true;
        } else {
            auto temp_it = it_;
            while (temp_it != end_) {
                (*temp_it).clear();
                (*temp_it).seekg(0);
                ++temp_it;
            }
            FindNextToken();
        }
    }

    reference operator*() { return current_token_; }
    pointer operator->() { return &current_token_; }

    SplitIterator& operator++() {
        FindNextToken();
        return *this;
    }

    bool operator==(const SplitIterator& other) const {
        return at_end_ == other.at_end_;
    }

    bool operator!=(const SplitIterator& other) const {
        return !(*this == other);
    }

private:
    RangeIterator it_;
    RangeSentinel end_;
    std::string delimiters_;
    std::string current_token_;
    bool at_end_ = false;

    void FindNextToken() {
        current_token_.clear();
        while (it_ != end_) {
            std::istream& current_stream = *it_;

            char c;
            while (current_stream.get(c)) {
                if (delimiters_.find(c) != std::string::npos) {
                    return;
                }
                current_token_ += c;
            }

            if (!current_token_.empty()) {
                return;
            }

            ++it_; 
        }
        at_end_ = true;
    }
};

template <IsRange Range>
class SplitView {
public:
    SplitView(Range range, std::string delimiters) : range_(std::move(range)), delimiters_(std::move(delimiters)) {}

    // NOLINTNEXTLINE
    auto begin() { return SplitIterator(std::begin(range_), std::end(range_), delimiters_); }
    // NOLINTNEXTLINE
    auto end() { return SplitIterator<decltype(std::begin(range_)), decltype(std::end(range_))>(); }

private:
    Range range_;
    std::string delimiters_;
};

template <typename Delimiters>
struct SplitClosure : Pipeable<SplitClosure<Delimiters>> {
    Delimiters delimiters_;
    explicit SplitClosure(Delimiters delimiters) : delimiters_(std::move(delimiters)) {}

    template <IsRange Range>
    auto operator()(Range&& range) const {
        return SplitView<std::decay_t<Range>>(std::forward<Range>(range), std::string(delimiters_));
    }
};

template <typename Delimiters>
auto Split(Delimiters&& delimiters) {
    return SplitClosure<std::decay_t<Delimiters>>(std::forward<Delimiters>(delimiters));
}