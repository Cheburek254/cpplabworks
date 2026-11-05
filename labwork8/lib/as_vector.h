#pragma once

#include "pipe.h"
#include <vector>
#include <iterator>
#include <type_traits>
#include <utility>

struct AsVectorClosure : Pipeable<AsVectorClosure> {
    template <IsRange Range>
    auto operator()(Range&& range) const {
        using value_type = std::decay_t<decltype(*std::begin(range))>;
        std::vector<value_type> result;
        for (auto&& elem : range) {
            result.push_back(std::forward<decltype(elem)>(elem));
        }
        return result;
    }
};

inline auto AsVector() {
    return AsVectorClosure{};
}