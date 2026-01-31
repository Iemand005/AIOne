#pragma once

#include <cstdint>

// #include <llama-cpp.h>

struct TextGenerationStats
{
    size_t tokensEvaluated;
    size_t tokensGenerated;
    size_t tokensCached;
    std::string output;
};
