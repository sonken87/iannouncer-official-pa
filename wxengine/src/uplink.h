#pragma once
#include <string>

#include "config.h"
#include "types.h"

namespace wxe {

// Serializes route winds into the wind-uplink files consumed by add-on
// aircraft, plus a plain JSON dump for inspection.
class WindUplink {
 public:
  explicit WindUplink(const Config& cfg) : cfg_(cfg) {}

  // Writes out/route-winds.json and, when the aircraft directories are
  // configured, the PMDG and iFly uplink files. Returns a human-readable
  // summary of what was written. Throws on I/O errors.
  std::string write(const RouteWinds& winds);

  // Exposed for tests: builds the generic JSON payload.
  static std::string to_json(const RouteWinds& winds);

 private:
  Config cfg_;
};

}  // namespace wxe
