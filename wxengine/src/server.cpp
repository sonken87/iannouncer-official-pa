#include "server.h"

#include <httplib.h>

#include <nlohmann/json.hpp>

#include "geo.h"

namespace wxe {

LocalServer::LocalServer(const Config& cfg, SimClient& sim, WindProvider& winds)
    : cfg_(cfg), sim_(sim), winds_(winds) {}

void LocalServer::run() {
  auto* srv = new httplib::Server();
  impl_ = srv;

  srv->Get("/health", [](const httplib::Request&, httplib::Response& res) {
    res.set_content("{\"ok\":true}", "application/json");
  });

  srv->Get("/status", [this](const httplib::Request&, httplib::Response& res) {
    AircraftState st = sim_.state();
    nlohmann::json j;
    j["connected"] = st.connected;
    j["valid"] = st.valid;
    j["lat"] = st.pos.lat;
    j["lon"] = st.pos.lon;
    j["altitude_ft"] = st.altitude_ft;
    j["wind_source"] = winds_.name();
    res.set_content(j.dump(), "application/json");
  });

  srv->Get("/api/v1/weather/current", [this](const httplib::Request&, httplib::Response& res) {
    AircraftState st = sim_.state();
    if (!st.valid) {
      res.status = 503;
      res.set_content("{\"error\":\"no aircraft data\"}", "application/json");
      return;
    }
    Waypoint here;
    here.ident = "ACFT";
    here.pos = st.pos;
    here.altitude_ft = static_cast<int>(st.altitude_ft);
    FlightPlan fp;
    fp.fixes.push_back(here);
    RouteWinds rw = winds_.fetch_route_winds(fp, cfg_.wind_levels_ft);
    nlohmann::json j;
    j["source"] = rw.source;
    if (!rw.fixes.empty()) {
      j["levels"] = nlohmann::json::array();
      for (const auto& l : rw.fixes[0].levels)
        j["levels"].push_back({{"altitude_ft", l.altitude_ft},
                               {"direction_deg", l.direction_deg},
                               {"speed_kt", l.speed_kt},
                               {"temperature_c", l.temperature_c}});
    }
    res.set_content(j.dump(), "application/json");
  });

  srv->listen(cfg_.server_host.c_str(), cfg_.server_port);
}

void LocalServer::stop() {
  if (impl_) static_cast<httplib::Server*>(impl_)->stop();
}

}  // namespace wxe
