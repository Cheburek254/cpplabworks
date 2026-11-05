#pragma once

#include "pipe.h"
#include "as_data_flow.h"
#include <cstddef>
#include <unordered_map>
#include <vector>
#include <utility>
#include <functional>
#include <type_traits>

template <typename InitialValue, typename Aggregator, typename KeySelector>
struct AggregateByKeyClosure : Pipeable<AggregateByKeyClosure<InitialValue, Aggregator, KeySelector>> {
    InitialValue initial_value_;
    Aggregator aggregator_;
    KeySelector key_selector_;

    AggregateByKeyClosure(InitialValue initial_value, Aggregator aggregator, KeySelector key_selector)
        : initial_value_(std::move(initial_value)),
          aggregator_(std::move(aggregator)),
          key_selector_(std::move(key_selector)) {}

    template <IsRange Range>
    auto operator()(Range&& range) const {
        
        using Element = std::decay_t<decltype(*std::begin(range))>;
        using Key = std::remove_cvref_t<std::invoke_result_t<KeySelector, const Element&>>;
        using Value = std::decay_t<InitialValue>;

        std::vector<std::pair<Key, Value>> result_vector;
        std::unordered_map<Key, std::size_t> key_to_index;
        
        for (auto&& element : range) {
            Key key = std::invoke(key_selector_, element);
            
            auto it = key_to_index.find(key);
            if (it == key_to_index.end()) {
                std::size_t idx = result_vector.size();
                result_vector.emplace_back(key, initial_value_);
                key_to_index.emplace(std::move(key), idx);
                std::invoke(aggregator_, element, result_vector[idx].second);
            } else {
                std::invoke(aggregator_, element, result_vector[it->second].second);
            }
        }

        return AsDataFlow(std::move(result_vector));
    }
};

template <typename InitialValue, typename Aggregator, typename KeySelector>
auto AggregateByKey(InitialValue&& initial_value, Aggregator aggregator, KeySelector key_selector) {
    return AggregateByKeyClosure<std::decay_t<InitialValue>, Aggregator, KeySelector>(
        std::forward<InitialValue>(initial_value),
        std::move(aggregator),
        std::move(key_selector)
    );
}