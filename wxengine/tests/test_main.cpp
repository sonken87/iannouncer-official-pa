#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <cmath>

#include "airports.h"
#include "geo.h"
#include "metar.h"
#include "open_meteo.h"
#include "simbrief.h"
#include "uplink.h"

using namespace wxe;

TEST_CASE("great-circle distance is symmetric and known") {
  GeoPoint jfk{40.6413, -73.7781};
  GeoPoint lax{33.9416, -118.4085};
  double d = distance_nm(jfk, lax);
  CHECK(d == doctest::Approx(2143).epsilon(0.02));
  CHECK(distance_nm(jfk, lax) == doctest::Approx(distance_nm(lax, jfk)));
}

TEST_CASE("wind vector round-trips") {
  double u, v, dir, spd;
  wind_to_uv(270, 50, u, v);   // wind from the west
  CHECK(u == doctest::Approx(50).epsilon(0.001));  // blows towards +east
  CHECK(v == doctest::Approx(0).epsilon(0.001));
  uv_to_wind(u, v, dir, spd);
  CHECK(dir == doctest::Approx(270));
  CHECK(spd == doctest::Approx(50));
}

TEST_CASE("wind interpolation blends direction as a vector") {
  std::vector<WindSample> prof = {{10000, 270, 20, -5}, {20000, 290, 60, -25}};
  WindSample out;
  REQUIRE(interpolate_profile(prof, 15000, out));
  CHECK(out.speed_kt > 20);
  CHECK(out.speed_kt < 60);
  CHECK(out.direction_deg > 270);
  CHECK(out.direction_deg < 290);
  CHECK(out.temperature_c == doctest::Approx(-15));
}

TEST_CASE("pressure level maps to sensible altitude") {
  CHECK(nearest_pressure_level_hpa(0) == 1000);
  CHECK(nearest_pressure_level_hpa(18000) == 500);
  CHECK(nearest_pressure_level_hpa(39000) == 200);
}

TEST_CASE("METAR decode covers the essentials") {
  Metar m = parse_metar("KJFK 121651Z 28018G28KT 10SM BKN050 M03/M10 A3012");
  CHECK(m.station == "KJFK");
  REQUIRE(m.wind.has_value());
  CHECK(m.wind->direction_deg == 280);
  CHECK(m.wind->speed_kt == 18);
  CHECK(m.wind->gust_kt == 28);
  REQUIRE(m.temperature_c.has_value());
  CHECK(*m.temperature_c == -3);
  REQUIRE(m.dewpoint_c.has_value());
  CHECK(*m.dewpoint_c == -10);
  REQUIRE(m.clouds.size() == 1);
  CHECK(m.clouds[0].cover == "BKN");
  CHECK(m.clouds[0].base_ft_agl == 5000);
}

TEST_CASE("SimBrief OFP parses fixes and route") {
  const char* ofp = R"({
    "fetch": {"status": "Success"},
    "origin": {"icao_code": "EGLL"},
    "destination": {"icao_code": "KJFK"},
    "general": {"initial_altitude": "37000"},
    "navlog": {"fix": [
      {"ident": "LON", "pos_lat": "51.5", "pos_long": "-0.4", "altitude_feet": "0"},
      {"ident": "TOC", "pos_lat": "52.0", "pos_long": "-2.0", "altitude_feet": "37000"}
    ]}
  })";
  FlightPlan p = parse_simbrief_ofp(ofp);
  CHECK(p.origin == "EGLL");
  CHECK(p.destination == "KJFK");
  CHECK(p.cruise_altitude_ft == 37000);
  REQUIRE(p.fixes.size() == 2);
  CHECK(p.fixes[1].ident == "TOC");
  CHECK(p.fixes[1].pos.lat == doctest::Approx(52.0));
}

TEST_CASE("Open-Meteo fix JSON becomes a wind profile") {
  const char* j = R"({"current": {
    "wind_speed_500hPa": 60, "wind_direction_500hPa": 270, "temperature_500hPa": -20,
    "wind_speed_250hPa": 90, "wind_direction_250hPa": 280, "temperature_250hPa": -45
  }})";
  Waypoint wp;
  wp.ident = "TEST";
  WaypointWinds w = OpenMeteoProvider::parse_fix(wp, j, {18000, 34000});
  REQUIRE(w.levels.size() == 2);
  CHECK(w.levels[0].speed_kt > 0);
}

TEST_CASE("airport db resolves ICAO and aliases") {
  const char* j = R"({
    "airports": {"KJFK": {"y": 40.64, "x": -73.78}},
    "coords": {"WPT1": [10.0, 20.0, 500]},
    "aliases": {"JFK": "KJFK"}
  })";
  AirportDb db = AirportDb::from_json(j);
  REQUIRE(db.coord("KJFK").has_value());
  CHECK(db.coord("KJFK")->lat == doctest::Approx(40.64));
  REQUIRE(db.coord("JFK").has_value());
  CHECK(db.coord("JFK")->lon == doctest::Approx(-73.78));
  REQUIRE(db.coord("WPT1").has_value());
  CHECK_FALSE(db.coord("NOPE").has_value());
}

TEST_CASE("uplink JSON serialization is well-formed") {
  RouteWinds rw;
  rw.source = "open-meteo";
  WaypointWinds w;
  w.waypoint.ident = "TOC";
  w.levels.push_back({34000, 280, 90, -45});
  rw.fixes.push_back(w);
  std::string j = WindUplink::to_json(rw);
  CHECK(j.find("\"open-meteo\"") != std::string::npos);
  CHECK(j.find("TOC") != std::string::npos);
}
