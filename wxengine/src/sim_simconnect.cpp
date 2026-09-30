// MSFS 2024 SimConnect client. Compiled only on Windows with the MSFS SDK.
#include <windows.h>

#include <SimConnect.h>

#include <atomic>
#include <mutex>

#include "geo.h"
#include "sim.h"

namespace wxe {

namespace {

enum DataDefinition : SIMCONNECT_DATA_DEFINITION_ID { DEF_AIRCRAFT = 1 };
enum DataRequest : SIMCONNECT_DATA_REQUEST_ID { REQ_AIRCRAFT = 1 };

#pragma pack(push, 1)
struct AircraftData {
  double lat;         // degrees
  double lon;         // degrees
  double alt;         // feet
  double heading;     // degrees true
  double on_ground;   // bool
};
#pragma pack(pop)

class SimConnectClient : public SimClient {
 public:
  ~SimConnectClient() override { disconnect(); }

  bool connect() override {
    std::lock_guard<std::mutex> lk(mu_);
    if (hsim_) return true;
    if (SUCCEEDED(SimConnect_Open(&hsim_, "wxengine", nullptr, 0, nullptr, 0))) {
      SimConnect_AddToDataDefinition(hsim_, DEF_AIRCRAFT, "PLANE LATITUDE", "degrees");
      SimConnect_AddToDataDefinition(hsim_, DEF_AIRCRAFT, "PLANE LONGITUDE", "degrees");
      SimConnect_AddToDataDefinition(hsim_, DEF_AIRCRAFT, "PLANE ALTITUDE", "feet");
      SimConnect_AddToDataDefinition(hsim_, DEF_AIRCRAFT, "PLANE HEADING DEGREES TRUE", "degrees");
      SimConnect_AddToDataDefinition(hsim_, DEF_AIRCRAFT, "SIM ON GROUND", "bool");
      SimConnect_RequestDataOnSimObject(hsim_, REQ_AIRCRAFT, DEF_AIRCRAFT,
                                        SIMCONNECT_OBJECT_ID_USER, SIMCONNECT_PERIOD_SECOND);
      state_.connected = true;
      return true;
    }
    hsim_ = nullptr;
    return false;
  }

  void disconnect() override {
    std::lock_guard<std::mutex> lk(mu_);
    if (hsim_) {
      SimConnect_Close(hsim_);
      hsim_ = nullptr;
    }
    state_ = AircraftState{};
  }

  AircraftState state() override {
    pump();
    std::lock_guard<std::mutex> lk(mu_);
    return state_;
  }

  bool inject_winds(const WaypointWinds& winds) override {
    // MSFS 2024 no longer exposes the legacy SimConnect weather-set calls, so
    // live wind/cloud injection is delegated to the companion Community package
    // ("stratawx-injector" in the shipped product), fed over the local API.
    // Confirm the supported injection surface for your target build before
    // wiring this up.
    (void)winds;
    return false;
  }

 private:
  void pump() {
    std::lock_guard<std::mutex> lk(mu_);
    if (!hsim_) return;
    SimConnect_CallDispatch(hsim_, &SimConnectClient::dispatch, this);
  }

  static void CALLBACK dispatch(SIMCONNECT_RECV* data, DWORD, void* ctx) {
    auto* self = static_cast<SimConnectClient*>(ctx);
    if (data->dwID == SIMCONNECT_RECV_ID_SIMOBJECT_DATA) {
      auto* obj = reinterpret_cast<SIMCONNECT_RECV_SIMOBJECT_DATA*>(data);
      if (obj->dwRequestID == REQ_AIRCRAFT) {
        auto* a = reinterpret_cast<AircraftData*>(&obj->dwData);
        self->state_.pos = {a->lat, a->lon};
        self->state_.altitude_ft = a->alt;
        self->state_.heading_true_deg = a->heading;
        self->state_.on_ground = a->on_ground != 0.0;
        self->state_.valid = true;
      }
    } else if (data->dwID == SIMCONNECT_RECV_ID_QUIT) {
      self->state_ = AircraftState{};
    }
  }

  HANDLE hsim_ = nullptr;
  std::mutex mu_;
  AircraftState state_;
};

}  // namespace

std::unique_ptr<SimClient> make_sim_client() { return std::make_unique<SimConnectClient>(); }

}  // namespace wxe
