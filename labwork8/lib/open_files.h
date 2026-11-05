#pragma once

#include "pipe.h"
#include <cstddef>
#include <fstream>
#include <memory>
#include <iterator>
#include <utility>
#include <type_traits>

template <typename Iterator, typename Sentinel>
class OpenFilesIterator {
public:
    using iterator_category = std::input_iterator_tag;
    using value_type = std::ifstream;
    using difference_type = std::ptrdiff_t;
    using pointer = std::ifstream*;
    using reference = std::ifstream&;

    OpenFilesIterator() = default;

    OpenFilesIterator(Iterator it, Sentinel end)
        : it_(std::move(it)), end_(std::move(end)) {
        OpenCurrent();
    }

    reference operator*() const { return *stream_; }
    pointer operator->() const { return stream_.get(); }

    OpenFilesIterator& operator++() {
        ++it_;
        OpenCurrent();
        return *this;
    }

    bool operator==(const OpenFilesIterator& other) const { return it_ == other.it_; }
    bool operator!=(const OpenFilesIterator& other) const { return it_ != other.it_; }

private:
    Iterator it_;
    Sentinel end_;
    std::shared_ptr<std::ifstream> stream_;

    void OpenCurrent() {
        if (it_ != end_) {
            stream_ = std::make_shared<std::ifstream>(*it_);
        } else {
            stream_.reset();
        }
    }
};

template <IsRange Range>
class OpenFilesView {
public:
    explicit OpenFilesView(Range range) : range_(std::move(range)) {}

    // NOLINTNEXTLINE
    auto begin() {
        return OpenFilesIterator<decltype(std::begin(range_)), decltype(std::end(range_))>(
            std::begin(range_), std::end(range_));
    }

    // NOLINTNEXTLINE
    auto end() {
        return OpenFilesIterator<decltype(std::end(range_)), decltype(std::end(range_))>(
            std::end(range_), std::end(range_));
    }

private:
    Range range_;
};

struct OpenFilesClosure : Pipeable<OpenFilesClosure> {
    template <IsRange Range>
    auto operator()(Range&& range) const {
        return OpenFilesView<std::decay_t<Range>>(std::forward<Range>(range));
    }
};

inline auto OpenFiles() {
    return OpenFilesClosure{};
}