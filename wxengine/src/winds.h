#pragma once
#include <memory>
#include <string>

#include "http_client.h"
#include "types.h"

namespace wxe {

// A source of winds/temperatures aloft along a route.
class WindProvider {
 public:
  virtual ~WindProvider() = default;
  virtual std::string name() const = 0;
  // Returns winds for every fix at the given FMC levels. Throws on hard errors
  // (network, auth); a fix with no data gets an empty level list.
  virtual RouteWinds fetch_route_winds(const FlightPlan& plan,
                                       const std::vector<int>& levels_ft) = 0;
};

struct ProviderDeps {
  HttpClient* http = nullptr;
  // Open-Meteo
  std::string open_meteo_base_url;
  std::string open_meteo_model;
  // StrataWx (values come from the environment / official docs)
  std::string stratawx_base_url;
  std::string stratawx_license_key;
  std::string stratawx_machine_id;
};

std::unique_ptr<WindProvider> make_wind_provider(const std::string& source,
                                                 const ProviderDeps& deps);

}  // namespace wxe
