#pragma once
#include <optional>
#include <string>
#include <vector>

namespace wxe {

struct MetarWind {
  int direction_deg = 0;  // 0 for calm/variable
  int speed_kt = 0;
  int gust_kt = 0;        // 0 when no gust
  bool variable = false;
};

struct MetarCloud {
  std::string cover;   // FEW/SCT/BKN/OVC/CLR/SKC/NSC
  int base_ft_agl = 0;
};

// A minimal METAR decode covering the fields the weather engine needs.
struct Metar {
  std::string station;
  std::optional<MetarWind> wind;
  std::optional<int> visibility_m;   // meters; 9999 == 10km+
  std::vector<MetarCloud> clouds;
  std::optional<int> temperature_c;
  std::optional<int> dewpoint_c;
  std::optional<double> altimeter_hpa;
  bool cavok = false;
  std::string raw;
};

// Parses a raw METAR string. Unknown groups are ignored rather than rejected.
Metar parse_metar(const std::string& raw);

}  // namespace wxe
