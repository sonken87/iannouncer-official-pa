#include <windows.h>
#include <winhttp.h>

#include "http_client.h"

namespace wxe {

namespace {

std::wstring widen(const std::string& s) {
  if (s.empty()) return {};
  int n = MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), nullptr, 0);
  std::wstring w(n, L'\0');
  MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), w.data(), n);
  return w;
}

struct Handle {
  HINTERNET h = nullptr;
  explicit Handle(HINTERNET handle) : h(handle) {}
  ~Handle() {
    if (h) WinHttpCloseHandle(h);
  }
  Handle(const Handle&) = delete;
  Handle& operator=(const Handle&) = delete;
  explicit operator bool() const { return h != nullptr; }
};

std::string last_error(const char* what) {
  return std::string(what) + " failed (error " + std::to_string(GetLastError()) + ")";
}

class WinHttpClient : public HttpClient {
 public:
  explicit WinHttpClient(const std::string& user_agent)
      : session_(WinHttpOpen(widen(user_agent).c_str(), WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                             WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0)) {
    if (session_) WinHttpSetTimeouts(session_.h, 10000, 10000, 15000, 30000);
  }

  HttpResponse get(const std::string& url, const HttpHeaders& headers) override {
    HttpResponse res;
    if (!session_) {
      res.error = last_error("WinHttpOpen");
      return res;
    }

    std::wstring wurl = widen(url);
    URL_COMPONENTS uc{};
    uc.dwStructSize = sizeof(uc);
    uc.dwHostNameLength = static_cast<DWORD>(-1);
    uc.dwUrlPathLength = static_cast<DWORD>(-1);
    uc.dwExtraInfoLength = static_cast<DWORD>(-1);
    if (!WinHttpCrackUrl(wurl.c_str(), 0, 0, &uc)) {
      res.error = last_error("WinHttpCrackUrl");
      return res;
    }
    std::wstring host(uc.lpszHostName, uc.dwHostNameLength);
    std::wstring path(uc.lpszUrlPath, uc.dwUrlPathLength);
    if (uc.lpszExtraInfo) path.append(uc.lpszExtraInfo, uc.dwExtraInfoLength);

    Handle conn(WinHttpConnect(session_.h, host.c_str(), uc.nPort, 0));
    if (!conn) {
      res.error = last_error("WinHttpConnect");
      return res;
    }
    DWORD flags = uc.nScheme == INTERNET_SCHEME_HTTPS ? WINHTTP_FLAG_SECURE : 0;
    Handle req(WinHttpOpenRequest(conn.h, L"GET", path.c_str(), nullptr, WINHTTP_NO_REFERER,
                                  WINHTTP_DEFAULT_ACCEPT_TYPES, flags));
    if (!req) {
      res.error = last_error("WinHttpOpenRequest");
      return res;
    }

    std::wstring header_block;
    for (const auto& [k, v] : headers) header_block += widen(k) + L": " + widen(v) + L"\r\n";
    if (!header_block.empty() &&
        !WinHttpAddRequestHeaders(req.h, header_block.c_str(), static_cast<DWORD>(-1),
                                  WINHTTP_ADDREQ_FLAG_ADD | WINHTTP_ADDREQ_FLAG_REPLACE)) {
      res.error = last_error("WinHttpAddRequestHeaders");
      return res;
    }

    if (!WinHttpSendRequest(req.h, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0,
                            0, 0) ||
        !WinHttpReceiveResponse(req.h, nullptr)) {
      res.error = last_error("WinHttpSendRequest");
      return res;
    }

    DWORD status = 0;
    DWORD size = sizeof(status);
    WinHttpQueryHeaders(req.h, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                        WINHTTP_HEADER_NAME_BY_INDEX, &status, &size, WINHTTP_NO_HEADER_INDEX);
    res.status = static_cast<int>(status);

    for (;;) {
      DWORD avail = 0;
      if (!WinHttpQueryDataAvailable(req.h, &avail)) {
        res.error = last_error("WinHttpQueryDataAvailable");
        return res;
      }
      if (avail == 0) break;
      std::string chunk(avail, '\0');
      DWORD read = 0;
      if (!WinHttpReadData(req.h, chunk.data(), avail, &read)) {
        res.error = last_error("WinHttpReadData");
        return res;
      }
      res.body.append(chunk.data(), read);
    }
    return res;
  }

 private:
  Handle session_;
};

}  // namespace

std::unique_ptr<HttpClient> make_http_client(const std::string& user_agent) {
  return std::make_unique<WinHttpClient>(user_agent);
}

}  // namespace wxe
