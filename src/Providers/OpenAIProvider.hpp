#pragma once

#include "ILLMProvider.hpp"

class OpenAIProvider : public AIOne::ILLMProvider {
public:
    OpenAIProvider(
        std::string baseUrl,
        std::string apiKey);

    std::vector<AIOne::ModelInfo> getModels() override;

    std::vector<Message> complete(
        const std::string& model,
        const std::vector<Message>& messages) override;

private:
    std::string baseUrl;
    std::string apiKey;
};