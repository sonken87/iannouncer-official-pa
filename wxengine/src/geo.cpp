#include "geo.h"

#include <algorithm>
#include <cmath>

namespace wxe {

namespace {
constexpr double kEarthRadiusNm = 3440.065;
double rad(double deg) { return deg * kPi / 180.0; }
}  // namespace

double distance_nm(const GeoPoint& a, const GeoPoint& b) {
  double dlat = rad(b.lat - a.lat);
  double dlon = rad(b.lon - a.lon);
  double h = std::sin(dlat / 2) * std::sin(dlat / 2) +
             std::cos(rad(a.lat)) * std::cos(rad(b.lat)) * std::sin(dlon / 2) * std::sin(dlon / 2);
  return 2 * kEarthRadiusNm * std::asin(std::min(1.0, std::sqrt(h)));
}

void wind_to_uv(double direction_deg, double speed, double& u, double& v) {
  // Wind FROM direction d blows towards d + 180.
  u = -speed * std::sin(rad(direction_deg));
  v = -speed * std::cos(rad(direction_deg));
}

void uv_to_wind(double u, double v, double& direction_deg, double& speed) {
  speed = std::hypot(u, v);
  if (speed < 1e-9) {
    direction_deg = 0.0;
    speed = 0.0;
    return;
  }
  direction_deg = std::fmod(std::atan2(-u, -v) * 180.0 / kPi + 360.0, 360.0);
}

bool interpolate_profile(const std::vector<WindSample>& profile, double altitude_ft,
                         WindSample& out) {
  if (profile.empty()) return false;
  if (altitude_ft <= profile.front().altitude_ft) {
    out = profile.front();
    out.altitude_ft = altitude_ft;
    return true;
  }
  if (altitude_ft >= profile.back().altitude_ft) {
    out = profile.back();
    out.altitude_ft = altitude_ft;
    return true;
  }
  for (size_t i = 1; i < profile.size(); ++i) {
    const WindSample& lo = profile[i - 1];
    const WindSample& hi = profile[i];
    if (altitude_ft > hi.altitude_ft) continue;
    double span = hi.altitude_ft - lo.altitude_ft;
    double t = span > 0 ? (altitude_ft - lo.altitude_ft) / span : 0.0;
    double ulo, vlo, uhi, vhi;
    wind_to_uv(lo.direction_deg, lo.speed_kt, ulo, vlo);
    wind_to_uv(hi.direction_deg, hi.speed_kt, uhi, vhi);
    out.altitude_ft = altitude_ft;
    uv_to_wind(ulo + (uhi - ulo) * t, vlo + (vhi - vlo) * t, out.direction_deg, out.speed_kt);
    out.temperature_c = lo.temperature_c + (hi.temperature_c - lo.temperature_c) * t;
    return true;
  }
  return false;  // unreachable for a sorted profile
}

}  // namespace wxe
