#include <string>

struct HttpResponse {
    std::string body;
};

class HttpClient
{
public:

    HttpResponse Post(const std::string& url, const std::string& body, const Headers& headers);
};