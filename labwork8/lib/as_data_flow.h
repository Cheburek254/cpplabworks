#pragma once

#include <iterator>
#include <utility>
#include <type_traits>

template <typename Container>
class DataFlowView {
public:
    using value_type = typename std::iterator_traits<decltype(std::begin(std::declval<Container&>()))>::value_type;

    explicit DataFlowView(Container container) : container_(std::forward<Container>(container)) {}

    // NOLINTNEXTLINE
    auto begin() { return std::begin(container_); }
    // NOLINTNEXTLINE
    auto end() { return std::end(container_); }
    // NOLINTNEXTLINE
    auto begin() const { return std::begin(container_); }
    // NOLINTNEXTLINE
    auto end() const { return std::end(container_); }

private:
    Container container_;
};

template <typename Container>
auto AsDataFlow(Container&& container) {
    return DataFlowView<Container>(std::forward<Container>(container));
}