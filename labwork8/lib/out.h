#pragma once

#include "pipe.h"
#include <iostream>
#include <utility>


struct OutClosure : Pipeable<OutClosure> {
    std::ostream& os_;

    explicit OutClosure(std::ostream& os) : os_(os) {}

    template <typename Range>
    void operator()(Range&& range) const {
        for (auto&& item : range) {
            os_ << item << '\n';
        }
    }
};

inline auto Out(std::ostream& os) {
    return OutClosure{os};
}