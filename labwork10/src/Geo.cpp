#include "Geo.hpp"
#include "Types.hpp"
#include <cmath>
#include <numbers>
#include <string>

namespace valkey {

double CalculateDistance(const GeoPoint& p1, const GeoPoint& p2) {
  double d_lat = (p2.latitude_ - p1.latitude_) * std::numbers::pi / 180.0;
  double d_lon = (p2.longitude_ - p1.longitude_) * std::numbers::pi / 180.0;
  double lat1 = p1.latitude_ * std::numbers::pi / 180.0;
  double lat2 = p2.latitude_ * std::numbers::pi / 180.0;

  double a = std::pow(std::sin(d_lat / 2.0), 2.0) +
             std::pow(std::sin(d_lon / 2.0), 2.0) * std::cos(lat1) * std::cos(lat2);
  double c = 2.0 * std::asin(std::sqrt(a));
  return kEarthRadiusKm * 1000.0 * c;
}

double ConvertUnitToMeters(double value, const std::string& unit) {
  if (unit == "km") return value * 1000.0;
  if (unit == "mi") return value * 1609.34;
  if (unit == "ft") return value * 0.3048;
  return value;
}

double ConvertMetersToUnit(double meters, const std::string& unit) {
  if (unit == "km") return meters / 1000.0;
  if (unit == "mi") return meters / 1609.34;
  if (unit == "ft") return meters / 0.3048;
  return meters;
}

} // namespace valkey