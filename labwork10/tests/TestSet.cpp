#include "Database.hpp"
#include <gtest/gtest.h>
#include <string>

TEST(SetTest, AddAndMembers) {
  valkey::Database db;
  EXPECT_EQ(db.ProcessCommand({"SADD", "s1", "a", "b", "a"}), "(integer) 2");
  EXPECT_EQ(db.ProcessCommand({"SCARD", "s1"}), "(integer) 2");
}

TEST(SetTest, SremAndIsMember) {
  valkey::Database db;
  db.ProcessCommand({"SADD", "s1", "a", "b", "c"});
  EXPECT_EQ(db.ProcessCommand({"SREM", "s1", "a", "d"}), "(integer) 1");
  EXPECT_EQ(db.ProcessCommand({"SISMEMBER", "s1", "b"}), "(integer) 1");
  EXPECT_EQ(db.ProcessCommand({"SISMEMBER", "s1", "a"}), "(integer) 0");
}

TEST(SetTest, Operations) {
  valkey::Database db;
  db.ProcessCommand({"SADD", "s1", "a", "b"});
  db.ProcessCommand({"SADD", "s2", "b", "c"});
  
  std::string inter = db.ProcessCommand({"SINTER", "s1", "s2"});
  EXPECT_TRUE(inter.find("\"b\"") != std::string::npos);
  EXPECT_TRUE(inter.find("\"a\"") == std::string::npos);
  
  std::string diff = db.ProcessCommand({"SDIFF", "s1", "s2"});
  EXPECT_TRUE(diff.find("\"a\"") != std::string::npos);
  EXPECT_TRUE(diff.find("\"b\"") == std::string::npos);
  
  EXPECT_EQ(db.ProcessCommand({"SMOVE", "s1", "s2", "a"}), "(integer) 1");
  EXPECT_EQ(db.ProcessCommand({"SISMEMBER", "s1", "a"}), "(integer) 0");
  EXPECT_EQ(db.ProcessCommand({"SISMEMBER", "s2", "a"}), "(integer) 1");
}