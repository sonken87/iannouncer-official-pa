#include "stratawx_provider.h"

#include <stdexcept>
#include <utility>

namespace wxe {

StrataWxProvider::StrataWxProvider(HttpClient& http, std::string base_url, std::string license_key,
                                   std::string machine_id)
    : http_(http),
      base_url_(std::move(base_url)),
      license_key_(std::move(license_key)),
      machine_id_(std::move(machine_id)) {}

RouteWinds StrataWxProvider::fetch_route_winds(const FlightPlan& plan,
                                               const std::vector<int>& levels_ft) {
  (void)plan;
  (void)levels_ft;
  if (base_url_.empty()) {
    throw std::runtime_error("StrataWx base URL is not configured (see config.json)");
  }
  if (license_key_.empty() || machine_id_.empty()) {
    throw std::runtime_error(
        "StrataWx requires STRATAWX_LICENSE_KEY and STRATAWX_MACHINE_ID in the environment");
  }

  // Credentials are sent per the contract; header names come from the docs.
  // const HttpHeaders headers = {
  //     {"x-stratawx-license", license_key_},
  //     {"x-stratawx-machine-id", machine_id_},
  // };
  // const std::string url = base_url_ + "/api/v1/route";  // confirm path
  // HttpResponse r = http_.get(url, headers);
  // ... parse r.body into RouteWinds per the documented schema ...

  throw std::runtime_error(
      "StrataWx provider is not implemented yet: supply the official API endpoint, header "
      "names, and response schema, then complete stratawx_provider.cpp");
}

}  // namespace wxe
