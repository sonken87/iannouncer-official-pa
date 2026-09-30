#pragma once
#include "types.h"

namespace wxe {

constexpr double kPi = 3.14159265358979323846;
constexpr double kFeetPerMeter = 3.28083989501312;

// Great-circle distance in nautical miles.
double distance_nm(const GeoPoint& a, const GeoPoint& b);

// Converts a meteorological wind (from-direction, speed) into u/v components
// (u positive towards east, v positive towards north) and back.
void wind_to_uv(double direction_deg, double speed, double& u, double& v);
void uv_to_wind(double u, double v, double& direction_deg, double& speed);

// Interpolates a wind profile (sorted by altitude) to the requested altitude.
// Wind is interpolated as vectors so direction wraps correctly; outside the
// profile the nearest sample is used. Returns false for an empty profile.
bool interpolate_profile(const std::vector<WindSample>& profile, double altitude_ft,
                         WindSample& out);

}  // namespace wxe
