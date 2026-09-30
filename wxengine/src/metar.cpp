#include "metar.h"

#include <cctype>
#include <cstdlib>
#include <sstream>

namespace wxe {

namespace {

bool all_digits(const std::string& s, size_t from, size_t len) {
  if (from + len > s.size()) return false;
  for (size_t i = 0; i < len; ++i)
    if (!std::isdigit((unsigned char)s[from + i])) return false;
  return true;
}

}  // namespace

Metar parse_metar(const std::string& raw) {
  Metar m;
  m.raw = raw;
  std::istringstream ss(raw);
  std::string tok;
  int index = 0;
  while (ss >> tok) {
    if (tok == "METAR" || tok == "SPECI") continue;
    if (index == 0 && tok.size() == 4 &&
        std::isalpha((unsigned char)tok[0])) {  // station id
      m.station = tok;
      index++;
      continue;
    }
    index++;

    if (tok == "CAVOK") {
      m.cavok = true;
      continue;
    }
    // Wind: dddff(Ggg)KT  or  VRBffKT
    if (tok.size() >= 7 && tok.compare(tok.size() - 2, 2, "KT") == 0) {
      MetarWind w;
      std::string dir = tok.substr(0, 3);
      if (dir == "VRB") {
        w.variable = true;
      } else if (all_digits(dir, 0, 3)) {
        w.direction_deg = std::atoi(dir.c_str());
      } else {
        goto not_wind;
      }
      size_t p = 3;
      std::string spd;
      while (p < tok.size() && std::isdigit((unsigned char)tok[p])) spd += tok[p++];
      if (spd.empty()) goto not_wind;
      w.speed_kt = std::atoi(spd.c_str());
      if (p < tok.size() && tok[p] == 'G') {
        ++p;
        std::string g;
        while (p < tok.size() && std::isdigit((unsigned char)tok[p])) g += tok[p++];
        w.gust_kt = std::atoi(g.c_str());
      }
      m.wind = w;
      continue;
    }
  not_wind:;
    // Temperature/dewpoint: TT/DD, with M prefix for negatives.
    if (tok.find('/') != std::string::npos && tok.size() >= 3 &&
        (std::isdigit((unsigned char)tok[0]) || tok[0] == 'M')) {
      auto slash = tok.find('/');
      std::string t = tok.substr(0, slash);
      std::string d = tok.substr(slash + 1);
      auto to_int = [](std::string s) -> std::optional<int> {
        int sign = 1;
        if (!s.empty() && s[0] == 'M') {
          sign = -1;
          s.erase(0, 1);
        }
        if (s.empty()) return std::nullopt;
        for (char c : s)
          if (!std::isdigit((unsigned char)c)) return std::nullopt;
        return sign * std::atoi(s.c_str());
      };
      auto ti = to_int(t);
      auto di = to_int(d);
      if (ti) m.temperature_c = ti;
      if (di) m.dewpoint_c = di;
      if (ti || di) continue;
    }
    // Altimeter: Qhhhh (hPa) or Ahhhh (inHg*100)
    if (tok.size() == 5 && tok[0] == 'Q' && all_digits(tok, 1, 4)) {
      m.altimeter_hpa = std::atoi(tok.c_str() + 1);
      continue;
    }
    if (tok.size() == 5 && tok[0] == 'A' && all_digits(tok, 1, 4)) {
      m.altimeter_hpa = std::atoi(tok.c_str() + 1) / 100.0 * 33.8639;
      continue;
    }
    // Clouds: FEW/SCT/BKN/OVC + 3-digit height (hundreds of ft), or clear.
    if (tok == "CLR" || tok == "SKC" || tok == "NSC" || tok == "NCD") {
      m.clouds.push_back({tok, 0});
      continue;
    }
    if (tok.size() >= 6) {
      std::string cover = tok.substr(0, 3);
      if ((cover == "FEW" || cover == "SCT" || cover == "BKN" || cover == "OVC") &&
          all_digits(tok, 3, 3)) {
        MetarCloud c;
        c.cover = cover;
        c.base_ft_agl = std::atoi(tok.substr(3, 3).c_str()) * 100;
        m.clouds.push_back(c);
        continue;
      }
    }
    // Visibility in meters (4 digits) or 9999.
    if (tok.size() == 4 && all_digits(tok, 0, 4)) {
      m.visibility_m = std::atoi(tok.c_str());
      continue;
    }
  }
  return m;
}

}  // namespace wxe
