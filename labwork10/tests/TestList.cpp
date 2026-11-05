#include "Database.hpp"
#include <gtest/gtest.h>
#include <string>
TEST(ListTest, PushAndPop) {
  valkey::Database db;
  EXPECT_EQ(db.ProcessCommand({"LPUSH", "l1", "a", "b"}), "(integer) 2");
  EXPECT_EQ(db.ProcessCommand({"RPUSH", "l1", "c"}), "(integer) 3");
  EXPECT_EQ(db.ProcessCommand({"LLEN", "l1"}), "(integer) 3");
  
  EXPECT_EQ(db.ProcessCommand({"LPOP", "l1"}), "\"b\"");
}

TEST(ListTest, LRangeAndIndex) {
  valkey::Database db;
  db.ProcessCommand({"RPUSH", "l1", "a", "b", "c", "d"});
  std::string range_res = db.ProcessCommand({"LRANGE", "l1", "1", "-2"});
  EXPECT_TRUE(range_res.find("\"b\"") != std::string::npos);
  EXPECT_TRUE(range_res.find("\"c\"") != std::string::npos);
  
  EXPECT_EQ(db.ProcessCommand({"LINDEX", "l1", "0"}), "\"a\"");
  EXPECT_EQ(db.ProcessCommand({"LINDEX", "l1", "-1"}), "\"d\"");
}

TEST(ListTest, LSetAndLInsert) {
  valkey::Database db;
  db.ProcessCommand({"RPUSH", "l1", "a", "b", "c"});
  EXPECT_EQ(db.ProcessCommand({"LSET", "l1", "1", "x"}), "OK");
  EXPECT_EQ(db.ProcessCommand({"LINDEX", "l1", "1"}), "\"x\"");
  
  EXPECT_EQ(db.ProcessCommand({"LINSERT", "l1", "BEFORE", "c", "y"}), "(integer) 4");
  EXPECT_EQ(db.ProcessCommand({"LINDEX", "l1", "2"}), "\"y\"");
}
