#define CPPHTTPLIB_OPENSSL_SUPPORT
#include <httplib.h>

#include "http_client.h"

namespace wxe {

namespace {

class HttplibClient : public HttpClient {
 public:
  explicit HttplibClient(std::string user_agent) : user_agent_(std::move(user_agent)) {}

  HttpResponse get(const std::string& url, const HttpHeaders& headers) override {
    HttpResponse res;
    // Split "scheme://host[:port]" from the path + query.
    auto scheme_end = url.find("://");
    if (scheme_end == std::string::npos) {
      res.error = "invalid url: " + url;
      return res;
    }
    auto path_start = url.find('/', scheme_end + 3);
    std::string origin = path_start == std::string::npos ? url : url.substr(0, path_start);
    std::string path = path_start == std::string::npos ? "/" : url.substr(path_start);

    httplib::Client cli(origin);
    cli.set_connection_timeout(10);
    cli.set_read_timeout(30);
    cli.set_follow_location(true);
    httplib::Headers h{{"User-Agent", user_agent_}};
    for (const auto& [k, v] : headers) h.emplace(k, v);

    auto r = cli.Get(path, h);
    if (!r) {
      res.error = "http error: " + httplib::to_string(r.error());
      return res;
    }
    res.status = r->status;
    res.body = r->body;
    return res;
  }

 private:
  std::string user_agent_;
};

}  // namespace

std::unique_ptr<HttpClient> make_http_client(const std::string& user_agent) {
  return std::make_unique<HttplibClient>(user_agent);
}

}  // namespace wxe
