#include "simbrief.h"

#include <nlohmann/json.hpp>
#include <stdexcept>

namespace wxe {

namespace {

using nlohmann::json;

// SimBrief encodes numbers as strings; accept either.
double num(const json& j, const char* key, double fallback = 0.0) {
  if (!j.contains(key)) return fallback;
  const json& v = j.at(key);
  if (v.is_number()) return v.get<double>();
  if (v.is_string()) {
    try {
      return std::stod(v.get<std::string>());
    } catch (...) {
      return fallback;
    }
  }
  return fallback;
}

std::string str(const json& j, const char* key) {
  return j.contains(key) && j.at(key).is_string() ? j.at(key).get<std::string>() : std::string();
}

}  // namespace

FlightPlan parse_simbrief_ofp(const std::string& json_text) {
  json ofp;
  try {
    ofp = json::parse(json_text);
  } catch (const json::exception& e) {
    throw std::runtime_error(std::string("SimBrief response is not JSON: ") + e.what());
  }
  if (ofp.contains("fetch") && ofp["fetch"].is_object() && str(ofp["fetch"], "status") != "" &&
      str(ofp["fetch"], "status") != "Success") {
    throw std::runtime_error("SimBrief: " + str(ofp["fetch"], "status"));
  }
  if (!ofp.contains("navlog") || !ofp["navlog"].contains("fix")) {
    throw std::runtime_error("SimBrief OFP has no navlog");
  }

  FlightPlan plan;
  if (ofp.contains("origin")) plan.origin = str(ofp["origin"], "icao_code");
  if (ofp.contains("destination")) plan.destination = str(ofp["destination"], "icao_code");
  if (ofp.contains("general")) {
    plan.cruise_altitude_ft = static_cast<int>(num(ofp["general"], "initial_altitude"));
  }

  const json& fixes = ofp["navlog"]["fix"];
  // A single-fix navlog is an object rather than an array.
  auto add = [&](const json& f) {
    Waypoint wp;
    wp.ident = str(f, "ident");
    wp.pos.lat = num(f, "pos_lat");
    wp.pos.lon = num(f, "pos_long");
    wp.altitude_ft = static_cast<int>(num(f, "altitude_feet"));
    plan.fixes.push_back(wp);
  };
  if (fixes.is_array()) {
    for (const auto& f : fixes) add(f);
  } else if (fixes.is_object()) {
    add(fixes);
  }
  if (plan.fixes.empty()) throw std::runtime_error("SimBrief navlog is empty");
  return plan;
}

FlightPlan fetch_simbrief_ofp(HttpClient& http, const std::string& userid,
                              const std::string& username) {
  std::string url = "https://www.simbrief.com/api/xml.fetcher.php?json=1&";
  if (!userid.empty()) {
    url += "userid=" + url_encode(userid);
  } else if (!username.empty()) {
    url += "username=" + url_encode(username);
  } else {
    throw std::runtime_error("SimBrief user id or username is not configured");
  }
  HttpResponse r = http.get(url);
  if (!r.error.empty()) throw std::runtime_error("SimBrief request failed: " + r.error);
  // SimBrief answers 400 with a JSON body describing the problem (e.g. unknown user).
  return parse_simbrief_ofp(r.body);
}

}  // namespace wxe
