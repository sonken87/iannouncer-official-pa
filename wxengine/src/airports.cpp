#include "airports.h"

#include <fstream>
#include <nlohmann/json.hpp>
#include <sstream>
#include <stdexcept>

namespace wxe {

AirportDb AirportDb::from_json(const std::string& json_text) {
  nlohmann::json j;
  try {
    j = nlohmann::json::parse(json_text);
  } catch (const nlohmann::json::exception& e) {
    throw std::runtime_error(std::string("airport db is not JSON: ") + e.what());
  }
  AirportDb db;
  // "airports": { "ICAO": { "y": lat, "x": lon, ... }, ... }
  if (j.contains("airports")) {
    for (auto it = j["airports"].begin(); it != j["airports"].end(); ++it) {
      const auto& a = it.value();
      if (a.contains("y") && a.contains("x")) {
        db.coords_[it.key()] = GeoPoint{a["y"].get<double>(), a["x"].get<double>()};
      }
    }
  }
  // "coords": { "IDENT": [lat, lon, elev], ... } — coordinate-only fixes.
  if (j.contains("coords")) {
    for (auto it = j["coords"].begin(); it != j["coords"].end(); ++it) {
      const auto& a = it.value();
      if (a.is_array() && a.size() >= 2) {
        db.coords_[it.key()] = GeoPoint{a[0].get<double>(), a[1].get<double>()};
      }
    }
  }
  // "aliases": { "SHORT": "CANONICAL", ... }
  if (j.contains("aliases")) {
    for (auto it = j["aliases"].begin(); it != j["aliases"].end(); ++it) {
      if (it.value().is_string()) db.aliases_[it.key()] = it.value().get<std::string>();
    }
  }
  return db;
}

AirportDb AirportDb::load(const std::string& path) {
  std::ifstream in(path, std::ios::binary);
  if (!in) throw std::runtime_error("cannot open airport db: " + path);
  std::ostringstream ss;
  ss << in.rdbuf();
  std::string text = ss.str();
  // Strip a UTF-8 BOM if present.
  if (text.size() >= 3 && (unsigned char)text[0] == 0xEF && (unsigned char)text[1] == 0xBB &&
      (unsigned char)text[2] == 0xBF) {
    text.erase(0, 3);
  }
  return from_json(text);
}

std::optional<GeoPoint> AirportDb::coord(const std::string& ident) const {
  auto it = coords_.find(ident);
  if (it != coords_.end()) return it->second;
  auto a = aliases_.find(ident);
  if (a != aliases_.end()) {
    auto it2 = coords_.find(a->second);
    if (it2 != coords_.end()) return it2->second;
  }
  return std::nullopt;
}

}  // namespace wxe
