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
        auto res = http.Get("/models", headers);
        if (res.status != 200) return {};

        std::vector<OpenAIModel> models;
        try {
            auto json = nlohmann::json::parse(res.body);
            if (!json.contains("data") || !json["data"].is_array()) return models;
            for (auto& item : json["data"]) {
                std::string id = item.value("id", "");
                if (id.empty()) continue;
                std::string ownedBy = item.value("owned_by", item.value("name", ""));
                models.push_back({ id, ownedBy });
            }
        } catch (...) {
            return {};
        }
        return models;
    }
};
