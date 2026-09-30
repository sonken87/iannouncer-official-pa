#pragma once
#include <string>
#include <vector>

namespace wxe {

struct Config {
  std::string server_host = "127.0.0.1";
  int server_port = 5180;

  std::string wind_source = "open-meteo";  // "open-meteo" | "stratawx"
  std::string open_meteo_base_url = "https://api.open-meteo.com";
  std::string open_meteo_model = "gfs_seamless";

  // Filled in from the official StrataWx API documentation.
  std::string stratawx_base_url;

  std::string simbrief_userid;
  std::string simbrief_username;

  std::vector<int> wind_levels_ft = {5000, 10000, 18000, 24000, 30000, 34000, 38000, 41000};

  std::string pmdg737_dir;
  std::string ifly737max_dir;
  std::string json_dir = "out";

  std::string airport_db = "airport-db.data.json";
};

// Loads the JSON config file; missing keys keep their defaults.
// Throws std::runtime_error on unreadable or malformed files.
Config load_config(const std::string& path);

// Reads an environment variable, returning an empty string if unset.
std::string env_or_empty(const char* name);

}  // namespace wxe
