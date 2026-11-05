#include <gtest/gtest.h>
#include "../lib/pipe.h"
#include <vector>
#include <string>
#include <type_traits>


struct DummyClosure : Pipeable<DummyClosure> {
    template <typename Range>
    decltype(auto) operator()(Range&& range) const {
        return std::forward<Range>(range);
    }
};

TEST(PipeConceptTest, CompileTimeChecks) {
    static_assert(IsRange<std::vector<int>>, "std::vector is a range");
    static_assert(IsRange<std::string>, "std::string is a range");
    static_assert(IsRange<int[5]>, "C-array is a range");
    static_assert(IsRange<const char[6]>, "String literal is a range");

    static_assert(!IsRange<int>, "int is not a range");
    static_assert(!IsRange<int*>, "Raw pointer without bounds is not a range");
    static_assert(!IsRange<DummyClosure>, "PipeAdapter itself is not a range");
}

TEST(PipeConceptTest, LValueVectorPassing) {
    std::vector<int> vec = {1, 2, 3};
    auto& result = vec | DummyClosure{};
    
    EXPECT_EQ(&result, &vec); 
    EXPECT_EQ(result.size(), 3);
}

TEST(PipeConceptTest, RValueVectorPassing) {
    auto result = std::vector<int>{1, 2, 3} | DummyClosure{};
    EXPECT_EQ(result.size(), 3);
    EXPECT_EQ(result[0], 1);
    EXPECT_EQ(result[2], 3);
}

TEST(PipeConceptTest, CArrayPassing) {
    int arr[] = {1, 2, 3};
    auto& result = arr | DummyClosure{};
    EXPECT_EQ(result[0], 1);
    EXPECT_EQ(result[2], 3);
}

TEST(PipeConceptTest, StringLiteralPassing) {
    auto& result = "hello" | DummyClosure{};
    EXPECT_EQ(result[0], 'h');
    EXPECT_EQ(result[4], 'o');
}
