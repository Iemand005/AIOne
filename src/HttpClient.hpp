#pragma once

#include <string>
#include <httplib.h>

struct HttpResponse {
    int status = 0;
    std::string body;
    std::string error_str;
    httplib::Error error = httplib::Error::Success;
    int ssl_error = 0;
    uint64_t ssl_backend_error = 0;
};

class HttpClient {
    httplib::Client cli;
public:
    HttpClient(const std::string& host)
        : cli(host) {}

    HttpResponse Get(const std::string& path, const httplib::Headers& headers = {}) {
        auto res = cli.Get(path, headers);
        if (res) {
            return { res->status, res->body };
        }
        return { 0, "", httplib::to_string(res.error()), res.error(),
                 res.ssl_error(), res.ssl_backend_error() };
    }

    HttpResponse Post(const std::string& path, const std::string& body, const std::string& contentType, const httplib::Headers& headers = {}) {
        auto res = cli.Post(path, headers, body, contentType);
        if (res) {
            return { res->status, res->body };
        }
        return { 0, "", httplib::to_string(res.error()), res.error(),
                 res.ssl_error(), res.ssl_backend_error() };
    }
};
