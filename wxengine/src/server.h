#pragma once
#include <string>

#include "config.h"
#include "sim.h"
#include "winds.h"

namespace wxe {

// Local 127.0.0.1 HTTP API for in-sim panels and the UI. Read-only endpoints:
//   GET /health                  -> {"ok":true}
//   GET /status                  -> aircraft link + last route summary
//   GET /api/v1/weather/current  -> current aircraft position winds (if linked)
//   GET /api/v1/route            -> last computed route winds
class LocalServer {
 public:
  LocalServer(const Config& cfg, SimClient& sim, WindProvider& winds);

  // Blocks serving requests until stop() is called from another thread.
  void run();
  void stop();

 private:
  const Config& cfg_;
  SimClient& sim_;
  WindProvider& winds_;
  void* impl_ = nullptr;  // httplib::Server, hidden to keep the header light
};

}  // namespace wxe
