#pragma once

#include "pipe.h"
#include <cstddef>
#include <iterator>
#include <optional>
#include <unordered_map>
#include <utility>
#include <memory>
#include <functional>
#include <type_traits>

template <typename Key, typename Value>
struct KV {
    Key key;
    Value value;

    bool operator==(const KV& other) const {
        return key == other.key && value == other.value;
    }
};

template <typename Base, typename Joined>
struct JoinResult {
    Base base;
    std::optional<Joined> joined;

    bool operator==(const JoinResult& other) const {
        return base == other.base && joined == other.joined;
    }
};

template <typename BaseIterator, typename BaseSentinel, typename KeySelector, typename ValueSelectorBase, typename Map>
class JoinIterator;

template <IsRange BaseRange, typename KeySelector, typename ValueSelectorBase, typename Map>
class JoinView;


template <typename BaseIterator, typename BaseSentinel, typename KeySelector, typename ValueSelectorBase, typename Map>
class JoinIterator {
public:
    using iterator_category = std::input_iterator_tag;
    using BaseElemRef = typename std::iterator_traits<BaseIterator>::reference;
    using BaseValue = std::remove_cvref_t<std::invoke_result_t<ValueSelectorBase&, BaseElemRef>>;
    using JoinedValue = typename Map::mapped_type;
    using value_type = JoinResult<BaseValue, JoinedValue>;
    using difference_type = std::ptrdiff_t;
    using reference = const value_type&;
    using pointer = const value_type*;

    using MapIterator = typename Map::const_iterator;

    JoinIterator() : at_end_(true) {} // End iterator

    JoinIterator(BaseIterator base_it, BaseSentinel base_end, std::shared_ptr<const Map> map, KeySelector key_selector, ValueSelectorBase val_selector)
        : base_it_(std::move(base_it)),
          base_end_(std::move(base_end)),
          map_(std::move(map)),
          key_selector_(std::move(key_selector)),
          value_selector_base_(std::move(val_selector)),
          at_end_(base_it_ == base_end_) {
        if (!at_end_) {
            // Position at the first element
            FindMatchesForCurrentBase();
        }
    }

    reference operator*() const { return current_result_; }
    pointer operator->() const { return &current_result_; }

    JoinIterator& operator++() {
        Advance();
        return *this;
    }

    bool operator==(const JoinIterator& other) const {
        return at_end_ && other.at_end_;
    }
    bool operator!=(const JoinIterator& other) const { return !(*this == other); }

private:
    void FindMatchesForCurrentBase() {
        auto range = map_->equal_range(std::invoke(key_selector_, *base_it_));
        matches_it_ = range.first;
        matches_end_ = range.second;
        current_result_.base = std::invoke(value_selector_base_, *base_it_);

        if (matches_it_ != matches_end_) {
            current_result_.joined = matches_it_->second;
            has_match_for_current_base_ = true;
        } else {
            current_result_.joined = std::nullopt;
            has_match_for_current_base_ = false;
        }
    }

    void Advance() {
        if (at_end_) return;

        if (has_match_for_current_base_) {
            ++matches_it_;
            if (matches_it_ != matches_end_) {
                current_result_.joined = matches_it_->second;
                return;
            }
        }

        while (true) {
            ++base_it_;
            if (base_it_ == base_end_) {
                at_end_ = true;
                return;
            }
            FindMatchesForCurrentBase();
            return;
        }
    }

    BaseIterator base_it_;
    BaseSentinel base_end_;
    std::shared_ptr<const Map> map_;
    KeySelector key_selector_;
    ValueSelectorBase value_selector_base_;

    value_type current_result_;
    MapIterator matches_it_;
    MapIterator matches_end_;
    bool at_end_ = false;
    bool has_match_for_current_base_ = false;
};


template <IsRange BaseRange, typename KeySelector, typename ValueSelectorBase, typename Map>
class JoinView {
public:
    JoinView(BaseRange range, KeySelector selector, ValueSelectorBase val_selector, std::shared_ptr<const Map> map)
        : base_range_(std::move(range)), key_selector_(std::move(selector)), value_selector_base_(std::move(val_selector)), map_(std::move(map)) {}

    // NOLINTNEXTLINE
    auto begin() {
        return JoinIterator<decltype(std::begin(base_range_)), decltype(std::end(base_range_)), KeySelector, ValueSelectorBase, Map>(
            std::begin(base_range_), std::end(base_range_), map_, key_selector_, value_selector_base_);
    }

    // NOLINTNEXTLINE
    auto end() {
        return JoinIterator<decltype(std::begin(base_range_)), decltype(std::end(base_range_)), KeySelector, ValueSelectorBase, Map>();
    }

private:
    BaseRange base_range_;
    KeySelector key_selector_;
    ValueSelectorBase value_selector_base_;
    std::shared_ptr<const Map> map_;
};


template <IsRange JoinedRange, typename KeySelectorBase, typename KeySelectorJoined, typename ValueSelectorBase, typename ValueSelectorJoined>
struct JoinClosure : Pipeable<JoinClosure<JoinedRange, KeySelectorBase, KeySelectorJoined, ValueSelectorBase, ValueSelectorJoined>> {
    JoinedRange joined_range_;
    KeySelectorBase key_selector_base_;
    KeySelectorJoined key_selector_joined_;
    ValueSelectorBase value_selector_base_;
    ValueSelectorJoined value_selector_joined_;

    JoinClosure(JoinedRange joined_range, KeySelectorBase key_selector_base, KeySelectorJoined key_selector_joined, ValueSelectorBase value_selector_base, ValueSelectorJoined value_selector_joined)
        : joined_range_(std::move(joined_range)),
          key_selector_base_(std::move(key_selector_base)),
          key_selector_joined_(std::move(key_selector_joined)),
          value_selector_base_(std::move(value_selector_base)),
          value_selector_joined_(std::move(value_selector_joined)) {}

    template <IsRange BaseRange>
    auto operator()(BaseRange&& base_range) const {
        using BaseElemRef = decltype(*std::begin(base_range));
        using JoinedElemRef = decltype(*std::begin(joined_range_));
        using Key = std::remove_cvref_t<std::invoke_result_t<const KeySelectorBase&, BaseElemRef>>;
        using JoinedValue = std::remove_cvref_t<std::invoke_result_t<const ValueSelectorJoined&, JoinedElemRef>>;
        using Map = std::unordered_multimap<Key, JoinedValue>;

        auto map = std::make_shared<Map>();
        for (auto&& joined_item : joined_range_) {
            map->emplace(std::invoke(key_selector_joined_, joined_item), std::invoke(value_selector_joined_, joined_item));
        }

        return JoinView<std::decay_t<BaseRange>, KeySelectorBase, ValueSelectorBase, Map>(
            std::forward<BaseRange>(base_range), key_selector_base_, value_selector_base_, std::move(map));
    }
};

template <IsRange JoinedRange, typename KeySelectorBase, typename KeySelectorJoined>
auto Join(JoinedRange&& joined_range, KeySelectorBase key_selector_base, KeySelectorJoined key_selector_joined) {
    return JoinClosure<std::decay_t<JoinedRange>, KeySelectorBase, KeySelectorJoined, std::identity, std::identity>(
        std::forward<JoinedRange>(joined_range),
        std::move(key_selector_base),
        std::move(key_selector_joined),
        std::identity{},
        std::identity{}
    );
}

template <IsRange JoinedRange>
auto Join(JoinedRange&& joined_range) {
    auto key_sel = [](auto item) { return item.key; };
    auto val_sel = [](auto item) { return item.value; };
    return JoinClosure<std::decay_t<JoinedRange>, decltype(key_sel), decltype(key_sel), decltype(val_sel), decltype(val_sel)>(
        std::forward<JoinedRange>(joined_range),
        key_sel, key_sel, val_sel, val_sel
    );
}