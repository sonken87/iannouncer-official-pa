#pragma once
#include <string>

#include "http_client.h"
#include "types.h"

namespace wxe {

// Parses a SimBrief OFP in JSON form (xml.fetcher.php?...&json=1).
// Throws std::runtime_error when required fields are missing.
FlightPlan parse_simbrief_ofp(const std::string& json_text);

// Fetches the latest OFP for the given SimBrief user id or username
// (user id wins when both are set) and parses it.
FlightPlan fetch_simbrief_ofp(HttpClient& http, const std::string& userid,
                              const std::string& username);

}  // namespace wxe
