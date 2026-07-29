#pragma once

#include <string>
#include <vector>

// #include "../Opena"
#include "../Message.hpp"

namespace AIOne {
    struct ModelInfo {
        std::string id;
        std::string provider;
    };


    class ILLMProvider {
    public:
        virtual ~ILLMProvider() = default;

        virtual std::vector<ModelInfo> getModels() = 0;

        virtual std::string complete(
            const std::string& model,
            const std::vector<Message>& messages
        ) = 0;
    };
}