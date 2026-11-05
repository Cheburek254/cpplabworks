#include <circular_buffer.h>
#include <gtest/gtest.h>

struct no_default_constr_test_struct {
    no_default_constr_test_struct() = delete;
    explicit no_default_constr_test_struct(int) {}
};

TEST(NoDefaultConstructibleTest, CanConstruct) {
    circular_buffer<no_default_constr_test_struct> buffer(1);
    buffer.push_back(no_default_constr_test_struct(1));
    ASSERT_EQ(buffer.size(), 1);
}
