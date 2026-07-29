#pragma once

#include <string>
#include <httplib.h>

struct HttpResponse {
    int status = 0;
    std::string body;
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
        return { 0, "" };
    }

    HttpResponse Post(const std::string& path, const std::string& body, const std::string& contentType, const httplib::Headers& headers = {}) {
        auto res = cli.Post(path, headers, body, contentType);
        if (res) {
            return { res->status, res->body };
        }
        return { 0, "" };
    }
};
