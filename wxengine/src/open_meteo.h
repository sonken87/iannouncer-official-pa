#pragma once
#include <string>
#include <vector>

#include "http_client.h"
#include "winds.h"

namespace wxe {

// Winds aloft from the Open-Meteo pressure-level API (public data).
// Non-commercial use is free; commercial use needs an Open-Meteo plan.
class OpenMeteoProvider : public WindProvider {
 public:
  OpenMeteoProvider(HttpClient& http, std::string base_url, std::string model);

  std::string name() const override { return "open-meteo"; }
  RouteWinds fetch_route_winds(const FlightPlan& plan,
                               const std::vector<int>& levels_ft) override;

  // Exposed for tests: turns one fix's API JSON into a wind profile.
  static WaypointWinds parse_fix(const Waypoint& wp, const std::string& json_text,
                                 const std::vector<int>& levels_ft);

 private:
  HttpClient& http_;
  std::string base_url_;
  std::string model_;
};

// Standard pressure altitude (ft) -> pressure level (hPa) used by Open-Meteo.
int nearest_pressure_level_hpa(int altitude_ft);

}  // namespace wxe
