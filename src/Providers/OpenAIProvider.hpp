#pragma once

#include "ILLMProvider.hpp"

#include "..//HttpClient.hpp"

namespace AIOne {
    class OpenAIProvider : public ILLMProvider {
    public:
        OpenAIProvider(std::string baseUrl, std::string apiKey);

        std::vector<ModelInfo> getModels() override;

        std::vector<Message> complete(const std::string& model, const std::vector<Message>& messages) override;

    private:
		HttpClient http;
        std::string  apiKey;

    };
}