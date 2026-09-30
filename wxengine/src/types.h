#pragma once
#include <string>
#include <vector>

namespace wxe {

struct GeoPoint {
  double lat = 0.0;
  double lon = 0.0;
};

struct Waypoint {
  std::string ident;
  GeoPoint pos;
  int altitude_ft = 0;  // planned altitude at this fix
};

struct FlightPlan {
  std::string origin;
  std::string destination;
  int cruise_altitude_ft = 0;
  std::vector<Waypoint> fixes;  // origin..destination, in order
};

// One wind/temperature observation at a given altitude (MSL).
struct WindSample {
  double altitude_ft = 0.0;
  double direction_deg = 0.0;  // true, direction the wind blows FROM
  double speed_kt = 0.0;
  double temperature_c = 0.0;
};

struct WaypointWinds {
  Waypoint waypoint;
  std::vector<WindSample> levels;  // sorted by altitude, at the requested FMC levels
};

struct RouteWinds {
  std::string source;      // provider name
  long long valid_time = 0;  // unix seconds of the model hour used
  std::vector<WaypointWinds> fixes;
};

struct AircraftState {
  bool connected = false;
  bool valid = false;  // at least one data packet received
  GeoPoint pos;
  double altitude_ft = 0.0;
  double heading_true_deg = 0.0;
  bool on_ground = true;
};

}  // namespace wxe
