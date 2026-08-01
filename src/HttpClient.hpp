#pragma once

#include <string>
#include <httplib.h>

namespace {

struct UrlParts {
    std::string host;
    int port = 443;
    std::string basePath;
};

UrlParts parseUrl(const std::string& url) {
    UrlParts parts;
    std::string remaining = url;

    auto schemePos = remaining.find("://");
    if (schemePos != std::string::npos) {
        remaining = remaining.substr(schemePos + 3);
    }

    auto pathPos = remaining.find('/');
    std::string hostPart;
    if (pathPos != std::string::npos) {
        hostPart = remaining.substr(0, pathPos);
        parts.basePath = remaining.substr(pathPos);
    } else {
        hostPart = remaining;
        parts.basePath.clear();
    }

    if (parts.basePath.size() > 1 && parts.basePath.back() == '/') {
        parts.basePath.pop_back();
    }

    auto portPos = hostPart.find(':');
    if (portPos != std::string::npos) {
        parts.host = hostPart.substr(0, portPos);
        parts.port = std::stoi(hostPart.substr(portPos + 1));
    } else {
        parts.host = hostPart;
        parts.port = 443;
    }

    return parts;
}

}

struct HttpResponse {
    int status = 0;
    std::string body;
    std::string error_str;
    httplib::Error error = httplib::Error::Success;
    int ssl_error = 0;
    uint64_t ssl_backend_error = 0;
};

class HttpClient {
    httplib::SSLClient cli;
    std::string basePath;

    HttpClient(const UrlParts& parts)
        : cli(parts.host, parts.port), basePath(parts.basePath) {}

public:
    HttpClient(const std::string& url)
        : HttpClient(parseUrl(url)) {}

    HttpResponse Get(const std::string& path, const httplib::Headers& headers = {}) {
        auto res = cli.Get(basePath + path, headers);
        if (res) {
            return { res->status, res->body };
        }
        return { 0, "", httplib::to_string(res.error()), res.error(),
                 res.ssl_error(), res.ssl_backend_error() };
    }

    HttpResponse Post(const std::string& path, const std::string& body, const std::string& contentType, const httplib::Headers& headers = {}) {
        auto res = cli.Post(basePath + path, headers, body, contentType);
        if (res) {
            return { res->status, res->body };
        }
        return { 0, "", httplib::to_string(res.error()), res.error(),
                 res.ssl_error(), res.ssl_backend_error() };
    }

    HttpResponse PostStream(const std::string& path, const std::string& body, const std::string& contentType,
                            const httplib::Headers& headers, httplib::ContentReceiver onChunk) {
        auto res = cli.Post(basePath + path, headers, body, contentType, onChunk);
        if (!res) {
            return { 0, "", httplib::to_string(res.error()), res.error(),
                     res.ssl_error(), res.ssl_backend_error() };
        }
        return { res->status, res->body };
    }
};
