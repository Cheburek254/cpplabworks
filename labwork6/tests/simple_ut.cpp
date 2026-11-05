#include <circular_buffer.h>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <string>
#include <vector>
#include <cstddef>
#include <stdexcept> 

template <typename T>
class circ_buff_int_test_suite : public testing::Test {};

using circ_buff_int_test_types = testing::Types<
    circular_buffer<int, false>
    #ifdef RUN_EXT_TESTS
    , circular_buffer<int, true>
    #endif
>;

TYPED_TEST_SUITE(circ_buff_int_test_suite, circ_buff_int_test_types);

template <typename T>
class CircularBufferStringTestSuite : public testing::Test {};

using circ_buff_string_test_types = testing::Types<
    circular_buffer<std::string, false>
    #ifdef RUN_EXT_TESTS
    , circular_buffer<std::string, true>
    #endif
>;

TYPED_TEST_SUITE(CircularBufferStringTestSuite, circ_buff_string_test_types);

TYPED_TEST(circ_buff_int_test_suite, AlternatingPush) {
    TypeParam cb(6);
    for (int i = 0; i < 6; ++i) {
        if (i % 2 == 0) {
            cb.push_back(i);
        } else {
            cb.push_front(i);
        }
    }

    ASSERT_THAT(cb, testing::ElementsAre(5, 3, 1, 0, 2, 4));
}

TYPED_TEST(CircularBufferStringTestSuite, PushingComplicatedObjects) {
    TypeParam cb(3);
    cb.push_back("aaa");
    cb.push_back("bbb");
    cb.push_back("ccc");
    ASSERT_THAT(cb, testing::ElementsAre("aaa", "bbb", "ccc"));
}

TYPED_TEST(circ_buff_int_test_suite, SimplePopTest) {
    TypeParam cb(5);

    cb.push_back(1);
    cb.push_back(2);
    cb.push_back(3);

    cb.push_front(4);
    cb.push_front(5);

    cb.pop_back();
    cb.pop_front();

    ASSERT_THAT(cb, testing::ElementsAre(4, 1, 2));
}

TYPED_TEST(circ_buff_int_test_suite, PopFromEmpty) {
    TypeParam cb(1);
    cb.push_back(1);
    cb.pop_back();

    try {
        cb.pop_back();
        FAIL() ;
    } catch (const std::out_of_range&) {
        
    } catch (...) {
        FAIL();
    }

    try {
        cb.pop_front();
        FAIL();
    } catch (const std::out_of_range&) {
        
    } catch (...) {
        FAIL();
    }
}

TYPED_TEST(circ_buff_int_test_suite, EraseOneElement) {
    TypeParam cb = {1, 2, 3, 4, 5};

    cb.erase(cb.cbegin() + 2);

    ASSERT_THAT(cb, testing::ElementsAre(1, 2, 4, 5));
}

TYPED_TEST(circ_buff_int_test_suite, EraseSequence) {
    TypeParam cb(5);
    for (int i = 0; i < 5; ++i) {
        if (i % 2 == 0) {
            cb.push_back(i);
        } else {
            cb.push_front(i);
        }
    }
    cb.erase(cb.cbegin() + 1, cb.cend() - 1);

    ASSERT_THAT(cb, testing::ElementsAre(3, 4));
}

TYPED_TEST(circ_buff_int_test_suite, NValues) {
    TypeParam cb(5);

    cb.assign(static_cast<size_t>(5), 10);

    ASSERT_THAT(cb, testing::ElementsAre(10, 10, 10, 10, 10));
}

TYPED_TEST(circ_buff_int_test_suite, AssignIterator) {
    TypeParam cb(5);
    std::vector<int> v = {1, 2, 3, 4, 5};

    cb.assign(v.begin() + 1, v.end() - 1);

    ASSERT_THAT(cb, testing::ElementsAre(2, 3, 4));
}

TYPED_TEST(circ_buff_int_test_suite, InitializerList) {
    TypeParam cb = {5, 4, 3, 2, 1};

    cb.assign({1, 2, 3, 4, 5});

    ASSERT_THAT(cb, testing::ElementsAre(1, 2, 3, 4, 5));
}

TYPED_TEST(circ_buff_int_test_suite, SimpleTest) {
    TypeParam cb(5);
    for (int i = 0; i < 5; ++i) {
        if (i % 2 == 0) {
            cb.push_back(i);
        } else {
            cb.push_front(i);
        }
    }

    ASSERT_EQ(cb.front(), 3);
}
