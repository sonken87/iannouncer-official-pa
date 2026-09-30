#include <atomic>
#include <csignal>
#include <cstdio>
#include <iostream>
#include <string>
#include <thread>

#include "airports.h"
#include "config.h"
#include "http_client.h"
#include "server.h"
#include "sim.h"
#include "simbrief.h"
#include "uplink.h"
#include "winds.h"

namespace {
std::atomic<wxe::LocalServer*> g_server{nullptr};
void on_signal(int) {
  if (auto* s = g_server.load()) s->stop();
}
}  // namespace

static void usage() {
  std::puts(
      "wxengine - MSFS 2024 real-weather / winds-aloft engine\n"
      "Usage:\n"
      "  wxengine --config config.json serve         Run the local API server\n"
      "  wxengine --config config.json winds         Fetch SimBrief route winds and uplink\n"
      "Env: STRATAWX_LICENSE_KEY, STRATAWX_MACHINE_ID (required for --wind-source stratawx)");
}

int main(int argc, char** argv) {
  std::string config_path = "config.json";
  std::string command = "serve";
  for (int i = 1; i < argc; ++i) {
    std::string a = argv[i];
    if (a == "--config" && i + 1 < argc) {
      config_path = argv[++i];
    } else if (a == "serve" || a == "winds") {
      command = a;
    } else if (a == "-h" || a == "--help") {
      usage();
      return 0;
    }
  }

  wxe::Config cfg;
  try {
    cfg = wxe::load_config(config_path);
  } catch (const std::exception& e) {
    std::fprintf(stderr, "config: %s\n", e.what());
    return 2;
  }

  auto http = wxe::make_http_client("wxengine/0.1");
  wxe::ProviderDeps deps;
  deps.http = http.get();
  deps.open_meteo_base_url = cfg.open_meteo_base_url;
  deps.open_meteo_model = cfg.open_meteo_model;
  deps.stratawx_base_url = cfg.stratawx_base_url;
  deps.stratawx_license_key = wxe::env_or_empty("STRATAWX_LICENSE_KEY");
  deps.stratawx_machine_id = wxe::env_or_empty("STRATAWX_MACHINE_ID");

  std::unique_ptr<wxe::WindProvider> winds;
  try {
    winds = wxe::make_wind_provider(cfg.wind_source, deps);
  } catch (const std::exception& e) {
    std::fprintf(stderr, "wind provider: %s\n", e.what());
    return 2;
  }

  if (command == "winds") {
    try {
      wxe::FlightPlan plan =
          wxe::fetch_simbrief_ofp(*http, cfg.simbrief_userid, cfg.simbrief_username);
      std::printf("route %s -> %s, %zu fixes\n", plan.origin.c_str(), plan.destination.c_str(),
                  plan.fixes.size());
      wxe::RouteWinds rw = winds->fetch_route_winds(plan, cfg.wind_levels_ft);
      wxe::WindUplink up(cfg);
      std::printf("%s\n", up.write(rw).c_str());
      return 0;
    } catch (const std::exception& e) {
      std::fprintf(stderr, "winds: %s\n", e.what());
      return 1;
    }
  }

  auto sim = wxe::make_sim_client();
  sim->connect();  // best effort; server reports link status
  wxe::LocalServer server(cfg, *sim, *winds);
  g_server = &server;
  std::signal(SIGINT, on_signal);
  std::signal(SIGTERM, on_signal);
  std::printf("serving on http://%s:%d\n", cfg.server_host.c_str(), cfg.server_port);
  server.run();
  return 0;
}
