#pragma once

#include <cstdint>
#include "Message.hpp"

// #include <llama-cpp.h>

struct TextGenerationResult
{
    size_t tokensEvaluated;
    size_t tokensGenerated;
    size_t tokensCached;
    Message output;
};
