#include "Database.hpp"
#include <gtest/gtest.h>
#include <string>
TEST(GeneralTest, DelExistsType) {
  valkey::Database db;
  db.ProcessCommand({"SET", "k1", "v1"});
  db.ProcessCommand({"LPUSH", "k2", "v2"});
  EXPECT_EQ(db.ProcessCommand({"TYPE", "k1"}), "string");
  EXPECT_EQ(db.ProcessCommand({"DEL", "k1"}), "(integer) 1");
  EXPECT_EQ(db.ProcessCommand({"EXISTS", "k1"}), "(integer) 0");
}

TEST(GeneralTest, MaxMemory) {
  valkey::Database db;
  EXPECT_EQ(db.ProcessCommand({"CONFIG", "SET", "maxmemory", "10b"}), "OK");
  EXPECT_EQ(db.ProcessCommand({"SET", "k1", "very_long_string_value"}), 
            "(error) OOM command not allowed when used memory > 'maxmemory'");
}

TEST(GeneralTest, KeysAndDbsize) {
  valkey::Database db;
  db.ProcessCommand({"SET", "k1", "v1"});
  db.ProcessCommand({"SET", "k2", "v2"});
  db.ProcessCommand({"SET", "key3", "v3"});
  
  EXPECT_EQ(db.ProcessCommand({"DBSIZE"}), "(integer) 3");
  
  std::string keys = db.ProcessCommand({"KEYS", "k?"});
  EXPECT_TRUE(keys.find("k1") != std::string::npos);
  EXPECT_TRUE(keys.find("k2") != std::string::npos);
  EXPECT_TRUE(keys.find("key3") == std::string::npos);
  
  EXPECT_EQ(db.ProcessCommand({"FLUSHDB"}), "OK");
  EXPECT_EQ(db.ProcessCommand({"DBSIZE"}), "(integer) 0");
}