#include "winds.h"

#include <stdexcept>

#include "open_meteo.h"
#include "stratawx_provider.h"

namespace wxe {

std::unique_ptr<WindProvider> make_wind_provider(const std::string& source,
                                                 const ProviderDeps& deps) {
  if (deps.http == nullptr) throw std::runtime_error("wind provider needs an HTTP client");
  if (source == "open-meteo") {
    return std::make_unique<OpenMeteoProvider>(*deps.http, deps.open_meteo_base_url,
                                               deps.open_meteo_model);
  }
  if (source == "stratawx") {
    return std::make_unique<StrataWxProvider>(*deps.http, deps.stratawx_base_url,
                                              deps.stratawx_license_key, deps.stratawx_machine_id);
  }
  throw std::runtime_error("unknown wind source: " + source);
}

}  // namespace wxe
