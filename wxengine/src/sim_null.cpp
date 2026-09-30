#include "sim.h"

namespace wxe {

namespace {

// Used off-Windows and in tests: never links to a simulator.
class NullSimClient : public SimClient {
 public:
  bool connect() override { return false; }
  void disconnect() override {}
  AircraftState state() override { return AircraftState{}; }
  bool inject_winds(const WaypointWinds&) override { return false; }
};

}  // namespace

std::unique_ptr<SimClient> make_sim_client() { return std::make_unique<NullSimClient>(); }

}  // namespace wxe
