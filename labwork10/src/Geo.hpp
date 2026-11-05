#pragma once
#include "Types.hpp"
#include <string>

namespace valkey {

constexpr double kEarthRadiusKm = 6372.8;

double CalculateDistance(const GeoPoint& p1, const GeoPoint& p2);
double ConvertUnitToMeters(double value, const std::string& unit);
double ConvertMetersToUnit(double meters, const std::string& unit);

} // namespace valkey