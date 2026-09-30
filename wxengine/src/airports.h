#pragma once
#include <optional>
#include <string>
#include <unordered_map>

#include "types.h"

namespace wxe {

// Loads the OurAirports-derived airport-db.data.json (same file the shipped
// engine uses). Provides ICAO/alias lookup to fill in coordinates for fixes
// that carry only an ident.
class AirportDb {
 public:
  static AirportDb load(const std::string& path);

  std::optional<GeoPoint> coord(const std::string& ident) const;
  size_t size() const { return coords_.size(); }

  // Exposed for tests.
  static AirportDb from_json(const std::string& json_text);

 private:
  std::unordered_map<std::string, GeoPoint> coords_;
  std::unordered_map<std::string, std::string> aliases_;
};

}  // namespace wxe
