#pragma once
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace wxe {

struct HttpResponse {
  int status = 0;     // 0 when the request never got a response
  std::string body;
  std::string error;  // transport error, empty on success

  bool ok() const { return error.empty() && status >= 200 && status < 300; }
};

using HttpHeaders = std::vector<std::pair<std::string, std::string>>;

class HttpClient {
 public:
  virtual ~HttpClient() = default;
  virtual HttpResponse get(const std::string& url, const HttpHeaders& headers = {}) = 0;
};

// WinHTTP on Windows, cpp-httplib + OpenSSL elsewhere.
std::unique_ptr<HttpClient> make_http_client(const std::string& user_agent);

// Percent-encodes a query parameter value.
std::string url_encode(const std::string& s);

}  // namespace wxe
