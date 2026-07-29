#import <string>

class HttpClient
{
public:

    HttpResponse Post(const std::string& url, const std::string& body, const Headers& headers);
};