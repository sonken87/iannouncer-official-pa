#include "open_meteo.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <nlohmann/json.hpp>
#include <stdexcept>

#include "geo.h"

namespace wxe {

namespace {
using nlohmann::json;

// Open-Meteo offers winds on a fixed set of pressure levels (hPa).
constexpr std::array<int, 12> kLevelsHpa = {1000, 925, 850, 700, 600, 500,
                                            400,  300, 250, 200, 150, 100};

// ISA pressure altitude for a pressure level, in feet.
double isa_altitude_ft(int hpa) {
  // Below 226.32 hPa (~36,089 ft) use the troposphere formula, above it the
  // isothermal-layer formula.
  const double p = hpa;
  if (p > 226.32) {
    double m = 1.0 - std::pow(p / 1013.25, 0.190284);
    return 145366.45 * m;
  }
  return 36089.0 + std::log(226.32 / p) / 0.0000480637;
}
}  // namespace

int nearest_pressure_level_hpa(int altitude_ft) {
  int best = kLevelsHpa[0];
  double best_d = 1e18;
  for (int hpa : kLevelsHpa) {
    double d = std::abs(isa_altitude_ft(hpa) - altitude_ft);
    if (d < best_d) {
      best_d = d;
      best = hpa;
    }
  }
  return best;
}

OpenMeteoProvider::OpenMeteoProvider(HttpClient& http, std::string base_url, std::string model)
    : http_(http), base_url_(std::move(base_url)), model_(std::move(model)) {}

WaypointWinds OpenMeteoProvider::parse_fix(const Waypoint& wp, const std::string& json_text,
                                           const std::vector<int>& levels_ft) {
  WaypointWinds out;
  out.waypoint = wp;
  json j;
  try {
    j = json::parse(json_text);
  } catch (const json::exception& e) {
    throw std::runtime_error(std::string("Open-Meteo response is not JSON: ") + e.what());
  }
  if (j.contains("error") && j.value("error", false)) {
    throw std::runtime_error("Open-Meteo error: " + j.value("reason", std::string("unknown")));
  }
  if (!j.contains("current")) return out;  // no data for this point
  const json& cur = j["current"];

  // Build a full profile from all available pressure levels, then interpolate
  // to the requested FMC levels so uplink altitudes always resolve.
  std::vector<WindSample> profile;
  for (int hpa : kLevelsHpa) {
    std::string sd = "wind_direction_" + std::to_string(hpa) + "hPa";
    std::string ss = "wind_speed_" + std::to_string(hpa) + "hPa";
    std::string st = "temperature_" + std::to_string(hpa) + "hPa";
    if (!cur.contains(sd) || !cur.contains(ss)) continue;
    if (cur[sd].is_null() || cur[ss].is_null()) continue;
    WindSample s;
    s.altitude_ft = isa_altitude_ft(hpa);
    s.direction_deg = cur[sd].get<double>();
    s.speed_kt = cur[ss].get<double>();
    s.temperature_c = cur.contains(st) && !cur[st].is_null() ? cur[st].get<double>() : 0.0;
    profile.push_back(s);
  }
  std::sort(profile.begin(), profile.end(),
            [](const WindSample& a, const WindSample& b) { return a.altitude_ft < b.altitude_ft; });

  for (int lvl : levels_ft) {
    WindSample s;
    if (interpolate_profile(profile, lvl, s)) out.levels.push_back(s);
  }
  return out;
}

RouteWinds OpenMeteoProvider::fetch_route_winds(const FlightPlan& plan,
                                                const std::vector<int>& levels_ft) {
  RouteWinds rw;
  rw.source = name();

  // Request every pressure level once per fix; Open-Meteo bundles them per call.
  std::string vars;
  for (int hpa : kLevelsHpa) {
    std::string h = std::to_string(hpa);
    vars += "wind_speed_" + h + "hPa,wind_direction_" + h + "hPa,temperature_" + h + "hPa,";
  }
  if (!vars.empty()) vars.pop_back();

  for (const auto& wp : plan.fixes) {
    char coords[64];
    std::snprintf(coords, sizeof(coords), "latitude=%.5f&longitude=%.5f", wp.pos.lat, wp.pos.lon);
    std::string url = base_url_ + "/v1/forecast?" + coords + "&current=" + url_encode(vars) +
                      "&wind_speed_unit=kn";
    if (!model_.empty()) url += "&models=" + url_encode(model_);
    HttpResponse r = http_.get(url);
    if (!r.error.empty()) throw std::runtime_error("Open-Meteo request failed: " + r.error);
    if (r.status == 429) throw std::runtime_error("Open-Meteo rate limit reached (HTTP 429)");
    rw.fixes.push_back(parse_fix(wp, r.body, levels_ft));
  }
  return rw;
}

}  // namespace wxe
