#include <circular_buffer.h>
#include <gtest/gtest.h>
#include <cstdlib>
#include <concepts>
#include <type_traits>

struct node_type {};

struct observable_object {
    static int constructor_count;
    static int destructor_count;

    observable_object() {
        ++constructor_count;
    }

    ~observable_object() {
        ++destructor_count;
    }
};

int observable_object::constructor_count = 0;
int observable_object::destructor_count = 0;

template<class Alloc>
concept allocator_requirements = requires(Alloc alloc, std::size_t n) {
    { *alloc.allocate(n) } -> std::same_as<typename Alloc::value_type&>;
    { alloc.deallocate(alloc.allocate(n), n) };
} && std::copy_constructible<Alloc>
  && std::equality_comparable<Alloc>;

template<typename T>
class tracking_allocator {
public:
    using value_type = T;
    using pointer = T*;
    using size_type = size_t;
    using is_always_equal = std::true_type;

    static int allocation_count;
    static int elements_allocated;

    tracking_allocator() = default;

    template<typename U>
    tracking_allocator(const tracking_allocator<U>&) {}

    pointer allocate(size_type sz) {
        if constexpr (std::is_same_v<T, observable_object>) {
            ++tracking_allocator<observable_object>::allocation_count;
            tracking_allocator<observable_object>::elements_allocated += static_cast<int>(sz);
        } else {
            ++tracking_allocator<node_type>::allocation_count;
            tracking_allocator<node_type>::elements_allocated += static_cast<int>(sz);
        }
        return static_cast<pointer>(std::aligned_alloc(alignof(T), sizeof(T) * sz));
    }

    void deallocate(pointer p, std::size_t) {
        std::free(p);
    }

    bool operator==(const tracking_allocator&) const {
        return true;
    }

    bool operator!=(const tracking_allocator&) const {
        return false;
    }
};

template<typename T>
int tracking_allocator<T>::allocation_count = 0;

template<typename T>
int tracking_allocator<T>::elements_allocated = 0;

static_assert(allocator_requirements<tracking_allocator<observable_object>>);

class allocator_interaction_test : public testing::Test {
public:
    void SetUp() override {
        observable_object::constructor_count = 0;
        observable_object::destructor_count = 0;

        tracking_allocator<observable_object>::allocation_count = 0;
        tracking_allocator<observable_object>::elements_allocated = 0;

        tracking_allocator<node_type>::allocation_count = 0;
        tracking_allocator<node_type>::elements_allocated = 0;
    }
};

TEST_F(allocator_interaction_test, Reserve) {
    tracking_allocator<observable_object> allocator;
    circular_buffer<observable_object, false, tracking_allocator<observable_object>> buffer(5, allocator);

    ASSERT_EQ(tracking_allocator<observable_object>::allocation_count, 1);
    ASSERT_EQ(tracking_allocator<observable_object>::elements_allocated, 5);

    ASSERT_EQ(observable_object::constructor_count, 0);
    ASSERT_EQ(observable_object::destructor_count, 0);
}

TEST_F(allocator_interaction_test, SimplePushBack) {
    tracking_allocator<observable_object> allocator;
    circular_buffer<observable_object, false, tracking_allocator<observable_object>> buffer(5, allocator);
    for (int i = 0; i < 5; ++i) {
        buffer.push_back(observable_object{});
    }

    ASSERT_EQ(tracking_allocator<observable_object>::allocation_count, 1);
    ASSERT_EQ(tracking_allocator<observable_object>::elements_allocated, 5);

    ASSERT_EQ(observable_object::constructor_count, 5);
    ASSERT_EQ(observable_object::destructor_count, 5);
}

#ifdef RUN_EXT_TESTS
TEST_F(allocator_interaction_test, ExtendedPushBack) {
    tracking_allocator<observable_object> allocator;
    circular_buffer<observable_object, true, tracking_allocator<observable_object>> buffer(5, allocator);
    for (int i = 0; i < 6; ++i) {
        buffer.push_back(observable_object{});
    }

    ASSERT_EQ(tracking_allocator<observable_object>::allocation_count, 2);
    ASSERT_EQ(tracking_allocator<observable_object>::elements_allocated, 15);

    ASSERT_EQ(observable_object::constructor_count, 6);
    ASSERT_EQ(observable_object::destructor_count, 6);
}
#endif
