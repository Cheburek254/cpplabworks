#pragma once

#include "pipe.h"
#include <ostream>
#include <utility>
#include <iterator>
#include <type_traits>


template <typename Delimiter>
struct WriteClosure : Pipeable<WriteClosure<Delimiter>> {
    std::ostream& os_;
    Delimiter delimiter_;

    WriteClosure(std::ostream& os, Delimiter delimiter)
        : os_(os), delimiter_(std::move(delimiter)) {}

    template <IsRange Range>
    void operator()(Range&& range) const {
        for (auto&& item : range) {
            os_ << item << delimiter_;
        }
    }
};

template <typename Delimiter>
auto Write(std::ostream& os, Delimiter&& delimiter) {
    return WriteClosure<std::decay_t<Delimiter>>(os, std::forward<Delimiter>(delimiter));
}