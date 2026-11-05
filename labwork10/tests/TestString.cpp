#include "Database.hpp"
#include <gtest/gtest.h>
#include <string>
TEST(StringTest, SetAndGet) {
  valkey::Database db;
  EXPECT_EQ(db.ProcessCommand({"SET", "key1", "value1"}), "OK");
  EXPECT_EQ(db.ProcessCommand({"GET", "key1"}), "\"value1\"");
  EXPECT_EQ(db.ProcessCommand({"GET", "key2"}), "(nil)");
}

TEST(StringTest, StrlenAndAppend) {
  valkey::Database db;
  db.ProcessCommand({"SET", "k", "hello"});
  EXPECT_EQ(db.ProcessCommand({"STRLEN", "k"}), "(integer) 5");
  EXPECT_EQ(db.ProcessCommand({"APPEND", "k", " world"}), "(integer) 11");
  EXPECT_EQ(db.ProcessCommand({"GET", "k"}), "\"hello world\"");
}

TEST(StringTest, TTL) {
  valkey::Database db;
  db.ProcessCommand({"SET", "k", "val"});
  EXPECT_EQ(db.ProcessCommand({"TTL", "k"}), "(integer) -1");
  EXPECT_EQ(db.ProcessCommand({"EXPIRE", "k", "10"}), "(integer) 1");
}

TEST(StringTest, SetResetsTTL) {
  valkey::Database db;
  db.ProcessCommand({"SET", "k", "v"});
  db.ProcessCommand({"EXPIRE", "k", "100"});
  EXPECT_GT(std::stoi(db.ProcessCommand({"TTL", "k"}).substr(10)), 0);
  db.ProcessCommand({"SET", "k", "v2"});
  EXPECT_EQ(db.ProcessCommand({"TTL", "k"}), "(integer) -1");
}