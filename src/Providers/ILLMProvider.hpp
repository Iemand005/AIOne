#pragma once

#include <string>
#include <vector>

// #include "../Opena"
#include "../Message.hpp"

namespace AIOne {
    struct ModelInfo {
        std::string id, provider;
    };


    class ILLMProvider {
    public:
        virtual ~ILLMProvider() = default;

        virtual std::vector<ModelInfo> getModels() = 0;

        virtual std::vector<Message> complete(const std::string& model, const std::vector<Message>& messages) = 0;
    };
}