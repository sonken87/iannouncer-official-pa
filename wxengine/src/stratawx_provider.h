#pragma once
#include <string>

#include "http_client.h"
#include "winds.h"

namespace wxe {

// Winds aloft from the licensed StrataWx weather proxy.
//
// This talks to the official StrataWx API using the license key and machine id
// supplied by the operator. The request shape below (endpoint path, header
// names, JSON fields) is a PLACEHOLDER modelled on strings seen in the shipped
// client; fill it in from the official API documentation before use. Do not
// reverse-engineer or bypass the licensing.
class StrataWxProvider : public WindProvider {
 public:
  StrataWxProvider(HttpClient& http, std::string base_url, std::string license_key,
                   std::string machine_id);

  std::string name() const override { return "stratawx"; }
  RouteWinds fetch_route_winds(const FlightPlan& plan,
                               const std::vector<int>& levels_ft) override;

 private:
  HttpClient& http_;
  std::string base_url_;
  std::string license_key_;
  std::string machine_id_;
};

}  // namespace wxe
