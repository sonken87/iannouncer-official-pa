#pragma once
#include <memory>

#include "types.h"

namespace wxe {

// Abstraction over the flight simulator link. The real implementation talks to
// MSFS 2024 via SimConnect; a null implementation is used off-Windows and in
// tests so the rest of the engine can run without a simulator.
class SimClient {
 public:
  virtual ~SimClient() = default;
  virtual bool connect() = 0;
  virtual void disconnect() = 0;
  virtual AircraftState state() = 0;
  // Pushes a wind profile at the aircraft's position into the sim. Returns
  // false when not connected or the sim rejects the data.
  virtual bool inject_winds(const WaypointWinds& winds) = 0;
};

std::unique_ptr<SimClient> make_sim_client();

}  // namespace wxe
