#include <as_data_flow.h>
#include <write.h>

#include <gtest/gtest.h>

#include <sstream>
#include <vector>

TEST(WriteTest, Write) {
    std::vector<int> input = {1, 2, 3, 4, 5};
    std::stringstream file_emulator;
    AsDataFlow(input) | Write(file_emulator, '|');
    ASSERT_EQ(file_emulator.str(), "1|2|3|4|5|");
}
