#include "uplink.h"

#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <sstream>
#include <stdexcept>

namespace wxe {

namespace fs = std::filesystem;

std::string WindUplink::to_json(const RouteWinds& winds) {
  nlohmann::json j;
  j["source"] = winds.source;
  j["valid_time"] = winds.valid_time;
  j["fixes"] = nlohmann::json::array();
  for (const auto& f : winds.fixes) {
    nlohmann::json jf;
    jf["ident"] = f.waypoint.ident;
    jf["lat"] = f.waypoint.pos.lat;
    jf["lon"] = f.waypoint.pos.lon;
    jf["levels"] = nlohmann::json::array();
    for (const auto& l : f.levels) {
      jf["levels"].push_back({{"altitude_ft", l.altitude_ft},
                              {"direction_deg", l.direction_deg},
                              {"speed_kt", l.speed_kt},
                              {"temperature_c", l.temperature_c}});
    }
    j["fixes"].push_back(jf);
  }
  return j.dump(2);
}

namespace {

// PMDG and iFly consume winds as a compact per-waypoint table. The exact file
// layout is aircraft-specific and must be confirmed against each vendor's SDK;
// this writes a documented, self-describing text form as a starting point.
std::string vendor_table(const RouteWinds& winds, const char* vendor) {
  std::ostringstream os;
  os << "; wind uplink for " << vendor << "\n";
  os << "; source=" << winds.source << " valid_time=" << winds.valid_time << "\n";
  os << "; ident lat lon [alt_ft dir_deg spd_kt temp_c]...\n";
  for (const auto& f : winds.fixes) {
    os << f.waypoint.ident << ' ' << f.waypoint.pos.lat << ' ' << f.waypoint.pos.lon;
    for (const auto& l : f.levels) {
      os << ' ' << static_cast<int>(l.altitude_ft) << ' ' << static_cast<int>(l.direction_deg)
         << ' ' << static_cast<int>(l.speed_kt) << ' ' << static_cast<int>(l.temperature_c);
    }
    os << '\n';
  }
  return os.str();
}

void write_file(const fs::path& p, const std::string& content) {
  std::error_code ec;
  fs::create_directories(p.parent_path(), ec);
  std::ofstream out(p, std::ios::binary | std::ios::trunc);
  if (!out) throw std::runtime_error("cannot write " + p.string());
  out << content;
}

}  // namespace

std::string WindUplink::write(const RouteWinds& winds) {
  std::ostringstream summary;
  fs::path json_dir = cfg_.json_dir.empty() ? fs::path("out") : fs::path(cfg_.json_dir);
  fs::path json_path = json_dir / "route-winds.json";
  write_file(json_path, to_json(winds));
  summary << "wrote " << json_path.string();

  if (!cfg_.pmdg737_dir.empty()) {
    fs::path p = fs::path(cfg_.pmdg737_dir) / "wind_uplink.txt";
    write_file(p, vendor_table(winds, "PMDG 737"));
    summary << "; " << p.string();
  }
  if (!cfg_.ifly737max_dir.empty()) {
    fs::path p = fs::path(cfg_.ifly737max_dir) / "wind_uplink.txt";
    write_file(p, vendor_table(winds, "iFly 737 MAX"));
    summary << "; " << p.string();
  }
  return summary.str();
}

}  // namespace wxe
