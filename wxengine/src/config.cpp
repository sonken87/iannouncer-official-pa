#include "config.h"

#include <cstdlib>
#include <fstream>
#include <nlohmann/json.hpp>
#include <stdexcept>

namespace wxe {

namespace {

template <typename T>
void read(const nlohmann::json& j, const char* key, T& out) {
  if (j.contains(key) && !j.at(key).is_null()) out = j.at(key).get<T>();
}

}  // namespace

Config load_config(const std::string& path) {
  std::ifstream in(path);
  if (!in) throw std::runtime_error("cannot open config file: " + path);
  nlohmann::json j;
  try {
    in >> j;
  } catch (const nlohmann::json::exception& e) {
    throw std::runtime_error("invalid config file " + path + ": " + e.what());
  }

  Config c;
  const auto obj = [&](const char* key) {
    return j.contains(key) && j.at(key).is_object() ? j.at(key) : nlohmann::json::object();
  };
  const auto server = obj("server");
  read(server, "host", c.server_host);
  read(server, "port", c.server_port);
  read(j, "wind_source", c.wind_source);
  const auto om = obj("open_meteo");
  read(om, "base_url", c.open_meteo_base_url);
  read(om, "model", c.open_meteo_model);
  read(obj("stratawx"), "base_url", c.stratawx_base_url);
  const auto sb = obj("simbrief");
  read(sb, "userid", c.simbrief_userid);
  read(sb, "username", c.simbrief_username);
  read(obj("winds"), "levels_ft", c.wind_levels_ft);
  const auto up = obj("uplink");
  read(up, "pmdg737_dir", c.pmdg737_dir);
  read(up, "ifly737max_dir", c.ifly737max_dir);
  read(up, "json_dir", c.json_dir);
  read(j, "airport_db", c.airport_db);
  return c;
}

std::string env_or_empty(const char* name) {
#ifdef _WIN32
  char* buf = nullptr;
  size_t len = 0;
  if (_dupenv_s(&buf, &len, name) != 0 || buf == nullptr) return {};
  std::string value(buf);
  free(buf);
  return value;
#else
  const char* v = std::getenv(name);
  return v ? std::string(v) : std::string();
#endif
}

}  // namespace wxe
