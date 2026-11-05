#include "Database.hpp"
#include <gtest/gtest.h>
#include <string>

TEST(GeoTest, GeoDist) {
  valkey::Database db;
  EXPECT_EQ(db.ProcessCommand({"GEOADD", "g", "13.361389", "38.115556", "Palermo", "15.087269", "37.502669", "Catania"}), "(integer) 2");
  std::string dist = db.ProcessCommand({"GEODIST", "g", "Palermo", "Catania", "km"});
  EXPECT_TRUE(dist.find("166.") != std::string::npos);
}

TEST(GeoTest, GeoSearch) {
  valkey::Database db;
  db.ProcessCommand({"GEOADD", "g", "13.361389", "38.115556", "Palermo", "15.087269", "37.502669", "Catania"});
  
  std::string search = db.ProcessCommand({"GEOSEARCH", "g", "FROMLONLAT", "15", "37", "BYRADIUS", "500", "km"});
  EXPECT_TRUE(search.find("Palermo") != std::string::npos);
  EXPECT_TRUE(search.find("Catania") != std::string::npos);
  
  EXPECT_EQ(db.ProcessCommand({"GEOSEARCHSTORE", "g2", "g", "FROMLONLAT", "15", "37", "BYRADIUS", "500", "km"}), "(integer) 2");
  EXPECT_EQ(db.ProcessCommand({"TYPE", "g2"}), "zset");
}
