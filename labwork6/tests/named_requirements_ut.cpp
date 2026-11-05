#include <circular_buffer.h>
#include <gtest/gtest.h>
#include <concepts>
#include <initializer_list>

template<typename T>
concept container = requires(T a, const T b) {
    typename T::value_type;
    typename T::reference;
    typename T::const_reference;
    typename T::iterator;
    typename T::const_iterator;
    typename T::difference_type;
    typename T::size_type;

    { a.begin() } -> std::same_as<typename T::iterator>;
    { a.end() } -> std::same_as<typename T::iterator>;
    { b.begin() } -> std::same_as<typename T::const_iterator>;
    { b.end() } -> std::same_as<typename T::const_iterator>;
    { a.cbegin() } -> std::same_as<typename T::const_iterator>;
    { a.cend() } -> std::same_as<typename T::const_iterator>;
    { a.size() } -> std::same_as<typename T::size_type>;
    { a.max_size() } -> std::same_as<typename T::size_type>;
    { a.empty() } -> std::same_as<bool>;
    { a == b } -> std::same_as<bool>;
    { a != b } -> std::same_as<bool>;

    requires std::copy_constructible<T>;
    requires std::swappable<T>;
    requires std::default_initializable<T>;
    requires std::assignable_from<T&, T&>;
    requires std::assignable_from<T&, const T&>;
    requires std::assignable_from<T&, const T>;
};

template<typename T>
concept allocator_aware_container = requires(T a, typename T::allocator_type alloc) {
    typename T::allocator_type;

    requires std::constructible_from<T, typename T::allocator_type>;
    requires std::constructible_from<T, T&&, const typename T::allocator_type&>;

    { a.get_allocator() } -> std::same_as<typename T::allocator_type>;
};

template<typename T>
concept sequence_container = requires(T a, const T b, typename T::value_type value) {
    requires std::constructible_from<T, typename T::size_type, typename T::value_type>;
    requires std::constructible_from<T, std::initializer_list<typename T::value_type>>;
    requires std::assignable_from<T&, std::initializer_list<typename T::value_type>>;

    { a.front() } -> std::same_as<typename T::reference>;
    { b.front() } -> std::same_as<typename T::const_reference>;
    { a.back() } -> std::same_as<typename T::reference>;
    { b.back() } -> std::same_as<typename T::const_reference>;

    { a.push_front(value) } -> std::same_as<void>;
    { a.push_back(value) } -> std::same_as<void>;

    { a.pop_front() } -> std::same_as<void>;
    { a.pop_back() } -> std::same_as<void>;

    requires requires(typename T::const_iterator pos, const T::const_iterator pos2, typename T::size_type n) {
        { a.insert(pos, value) } -> std::same_as<typename T::iterator>;
        { a.insert(pos, n, value) } -> std::same_as<typename T::iterator>;
        { a.erase(pos) } -> std::same_as<typename T::iterator>;
        { a.erase(pos, pos2) } -> std::same_as<typename T::iterator>;
        { a.clear() };
    };
};

template<typename T>
concept reversible_container = requires(T a, const T b) {
    typename T::reverse_iterator;
    typename T::const_reverse_iterator;

    { a.rbegin() } -> std::same_as<typename T::reverse_iterator>;
    { a.rend() } -> std::same_as<typename T::reverse_iterator>;
    { b.rbegin() } -> std::same_as<typename T::const_reverse_iterator>;
    { b.rend() } -> std::same_as<typename T::const_reverse_iterator>;
    { a.crbegin() } -> std::same_as<typename T::const_reverse_iterator>;
    { a.crend() } -> std::same_as<typename T::const_reverse_iterator>;
};

TEST(NamedRequirementsTest, container) {
    static_assert(container<circular_buffer<int>>);
}

TEST(NamedRequirementsTest, allocator_aware_container) {
    static_assert(allocator_aware_container<circular_buffer<int>>);
}

TEST(NamedRequirementsTest, sequence_container) {
    static_assert(sequence_container<circular_buffer<int>>);
}

TEST(NamedRequirementsTest, reversible_container) {
    static_assert(reversible_container<circular_buffer<int>>);
}
