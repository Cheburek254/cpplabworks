#include <circular_buffer.h>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <vector>
#include <string>

template <typename T>
class circ_buff_circ_test_suite : public testing::Test {};

using circ_buff__test_types = testing::Types<
    circular_buffer<int, false>
    #ifdef RUN_EXT_TESTS
    , circular_buffer<int, true>
    #endif
>;

TYPED_TEST_SUITE(circ_buff_circ_test_suite, circ_buff__test_types);

TYPED_TEST(circ_buff_circ_test_suite, PushBackOverwritesOldest) {
    TypeParam cb(3);
    
    cb.push_back(1);
    cb.push_back(2);
    cb.push_back(3);
    
    cb.push_back(4);
    
    ASSERT_EQ(cb.size(), 3);
    ASSERT_THAT(cb, testing::ElementsAre(2, 3, 4));
    
    cb.push_back(5);
    
    ASSERT_EQ(cb.size(), 3);
    ASSERT_THAT(cb, testing::ElementsAre(3, 4, 5));
    
    cb.push_back(6);
    
    ASSERT_EQ(cb.size(), 3);
    ASSERT_THAT(cb, testing::ElementsAre(4, 5, 6));
}

TYPED_TEST(circ_buff_circ_test_suite, PushFrontOverwritesNewest) {
    TypeParam cb(3);
    
    cb.push_back(1);
    cb.push_back(2);
    cb.push_back(3);
    
    cb.push_front(4);
    
    ASSERT_EQ(cb.size(), 3);
    ASSERT_THAT(cb, testing::ElementsAre(4, 1, 2));
    
    cb.push_front(5);
    
    ASSERT_EQ(cb.size(), 3);
    ASSERT_THAT(cb, testing::ElementsAre(5, 4, 1));
    
    cb.push_front(6);
    
    ASSERT_EQ(cb.size(), 3);
    ASSERT_THAT(cb, testing::ElementsAre(6, 5, 4));
}

TYPED_TEST(circ_buff_circ_test_suite, MixedPushWhenFull) {
    TypeParam cb(3);
    
    cb.push_back(1);
    cb.push_back(2);
    cb.push_back(3);
    
    cb.push_front(4);
    ASSERT_THAT(cb, testing::ElementsAre(4, 1, 2));
    
    cb.push_back(5);
    ASSERT_THAT(cb, testing::ElementsAre(1, 2, 5));
    
    cb.push_front(6);
    ASSERT_THAT(cb, testing::ElementsAre(6, 1, 2));
}

TYPED_TEST(circ_buff_circ_test_suite, OrderAfterMultipleOperations) {
    TypeParam cb(4);
    
    for (int i = 1; i <= 8; ++i) {
        cb.push_back(i);
    }
    
    ASSERT_THAT(cb, testing::ElementsAre(5, 6, 7, 8));
    
    cb.pop_front();
    cb.push_back(9);
    
    ASSERT_THAT(cb, testing::ElementsAre(6, 7, 8, 9));
    
    cb.pop_back();
    cb.push_front(10);
    
    ASSERT_THAT(cb, testing::ElementsAre(10, 6, 7, 8));
}

TYPED_TEST(circ_buff_circ_test_suite, IterationAfterOverwrite) {
    TypeParam cb(3);
    
    cb.push_back(1);
    cb.push_back(2);
    cb.push_back(3);
    
    cb.push_back(4);
    cb.push_back(5);
    
    std::vector<int> result;
    for (auto it = cb.begin(); it != cb.end(); ++it) {
        result.push_back(*it);
    }
    
    ASSERT_THAT(result, testing::ElementsAre(3, 4, 5));
}

TYPED_TEST(circ_buff_circ_test_suite, FrontBackAfterOverwrite) {
    TypeParam cb(3);
    
    cb.push_back(1);
    cb.push_back(2);
    cb.push_back(3);
    
    ASSERT_EQ(cb.front(), 1);
    ASSERT_EQ(cb.back(), 3);
    
    cb.push_back(4);
    
    ASSERT_EQ(cb.front(), 2);
    ASSERT_EQ(cb.back(), 4);
    
    cb.push_front(5);
    
    ASSERT_EQ(cb.front(), 5);
    ASSERT_EQ(cb.back(), 3);
}

TYPED_TEST(circ_buff_circ_test_suite, CapacityOne) {
    TypeParam cb(1);
    
    cb.push_back(1);
    ASSERT_THAT(cb, testing::ElementsAre(1));
    
    cb.push_back(2);
    ASSERT_THAT(cb, testing::ElementsAre(2));
    
    cb.push_front(3);
    ASSERT_THAT(cb, testing::ElementsAre(3));
    
    ASSERT_EQ(cb.front(), 3);
    ASSERT_EQ(cb.back(), 3);
}

TYPED_TEST(circ_buff_circ_test_suite, CapacityZero) {
    TypeParam cb(0);
    
    cb.push_back(1);
    ASSERT_TRUE(cb.empty());
    ASSERT_EQ(cb.size(), 0);
    
    cb.push_front(2);
    ASSERT_TRUE(cb.empty());
    ASSERT_EQ(cb.size(), 0);
}

TEST(circ_buff_circ_test_suite, StringType) {
    circular_buffer<std::string, false> cb(3);
    
    cb.push_back("one");
    cb.push_back("two");
    cb.push_back("three");
    
    ASSERT_THAT(cb, testing::ElementsAre("one", "two", "three"));
    
    cb.push_back("four");
    
    ASSERT_THAT(cb, testing::ElementsAre("two", "three", "four"));
    
    cb.push_front("five");
    
    ASSERT_THAT(cb, testing::ElementsAre("five", "two", "three"));
}
TYPED_TEST(circ_buff_circ_test_suite, MoveConstructor) {
    TypeParam original(3);
    original.push_back(1);
    original.push_back(2);
    original.push_back(3);
    
    TypeParam moved(std::move(original));
    
    ASSERT_EQ(moved.size(), 3);
    ASSERT_THAT(moved, testing::ElementsAre(1, 2, 3));
    ASSERT_TRUE(original.empty());  
}

TYPED_TEST(circ_buff_circ_test_suite, MoveConstructorWithOverwrite) {
    TypeParam original(3);
    original.push_back(1);
    original.push_back(2);
    original.push_back(3);
    original.push_back(4);  
    
    TypeParam moved(std::move(original));
    
    ASSERT_EQ(moved.size(), 3);
    ASSERT_THAT(moved, testing::ElementsAre(2, 3, 4));
    ASSERT_TRUE(original.empty());
}


TYPED_TEST(circ_buff_circ_test_suite, PushBackLvalueWhenFull) {
    TypeParam cb(3);
    
    int a = 1, b = 2, c = 3, d = 4, e = 5;
    
    cb.push_back(a);
    cb.push_back(b);
    cb.push_back(c);
    
    ASSERT_THAT(cb, testing::ElementsAre(1, 2, 3));
    
    cb.push_back(d); 
    ASSERT_THAT(cb, testing::ElementsAre(2, 3, 4));
    
    cb.push_back(e);  
    ASSERT_THAT(cb, testing::ElementsAre(3, 4, 5));
    
   
    ASSERT_EQ(a, 1);
    ASSERT_EQ(b, 2);
    ASSERT_EQ(c, 3);
    ASSERT_EQ(d, 4);
    ASSERT_EQ(e, 5);
}


TYPED_TEST(circ_buff_circ_test_suite, PushFrontLvalueWhenFull) {
    TypeParam cb(3);
    
    int a = 1, b = 2, c = 3, d = 4, e = 5;
    
    cb.push_back(a);
    cb.push_back(b);
    cb.push_back(c);
    
    ASSERT_THAT(cb, testing::ElementsAre(1, 2, 3));
    
    cb.push_front(d);  
    ASSERT_THAT(cb, testing::ElementsAre(4, 1, 2));
    
    cb.push_front(e);  
    ASSERT_THAT(cb, testing::ElementsAre(5, 4, 1));
    
  
    ASSERT_EQ(a, 1);
    ASSERT_EQ(b, 2);
    ASSERT_EQ(c, 3);
    ASSERT_EQ(d, 4);
    ASSERT_EQ(e, 5);
}


TYPED_TEST(circ_buff_circ_test_suite, MixedLvalueOperationsWhenFull) {
    TypeParam cb(3);
    
    int a = 1, b = 2, c = 3, d = 4, e = 5, f = 6;
    
    cb.push_back(a);
    cb.push_back(b);
    cb.push_back(c);
    
    cb.push_front(d);
    ASSERT_THAT(cb, testing::ElementsAre(4, 1, 2));
    
    cb.push_back(e);
    ASSERT_THAT(cb, testing::ElementsAre(1, 2, 5));
    
    cb.push_front(f);
    ASSERT_THAT(cb, testing::ElementsAre(6, 1, 2));
    
 
    ASSERT_EQ(a, 1);
    ASSERT_EQ(b, 2);
    ASSERT_EQ(c, 3);
    ASSERT_EQ(d, 4);
    ASSERT_EQ(e, 5);
    ASSERT_EQ(f, 6);
}


TEST(circ_buff_circ_test_suite, StringTypeLvalueOperations) {
    circular_buffer<std::string, false> cb(3);
    
    std::string s1 = "one";
    std::string s2 = "two";
    std::string s3 = "three";
    std::string s4 = "four";
    std::string s5 = "five";
    
    cb.push_back(s1);
    cb.push_back(s2);
    cb.push_back(s3);
    
    ASSERT_THAT(cb, testing::ElementsAre("one", "two", "three"));
    
    cb.push_back(s4);  
    ASSERT_THAT(cb, testing::ElementsAre("two", "three", "four"));
    
    cb.push_front(s5); 
    ASSERT_THAT(cb, testing::ElementsAre("five", "two", "three"));
    

    ASSERT_EQ(s1, "one");
    ASSERT_EQ(s2, "two");
    ASSERT_EQ(s3, "three");
    ASSERT_EQ(s4, "four");
    ASSERT_EQ(s5, "five");
}


TEST(circ_buff_circ_test_suite, StringTypeMoveConstructor) {
    circular_buffer<std::string, false> original(3);
    original.push_back("one");
    original.push_back("two");
    original.push_back("three");
    
    circular_buffer<std::string, false> moved(std::move(original));
    
    ASSERT_EQ(moved.size(), 3);
    ASSERT_THAT(moved, testing::ElementsAre("one", "two", "three"));
    ASSERT_TRUE(original.empty());
}
