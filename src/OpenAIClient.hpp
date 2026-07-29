#pragma once

#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include "HttpClient.hpp"

struct OpenAIModel {
    std::string id;
    std::string ownedBy;
};

class OpenAIClient {
    HttpClient http;
    std::string apiKey;
public:
    OpenAIClient(const std::string& baseUrl, const std::string& apiKey)
        : http(baseUrl), apiKey(apiKey) {}

    std::vector<OpenAIModel> getModels() {
        httplib::Headers headers = {
            {"Authorization", "Bearer " + apiKey}
        };
        auto res = http.Get("/v1/models", headers);
        if (res.status != 200) return {};

        auto json = nlohmann::json::parse(res.body);
        std::vector<OpenAIModel> models;
        for (auto& item : json["data"]) {
            models.push_back({ item["id"], item["owned_by"] });
        }
        return models;
    }
};
