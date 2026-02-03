#pragma once

#include <cstdint>
#include <functional>

#include "TextGenResult.hpp"
#include "Callbacks.h"

typedef std::function<void(const std::string &token)> TokenCallback;
typedef std::function<void(const TextGenResult &output)> FinishCallback;

struct TextGenOptionsBase
{
    size_t maxTokens = 400;
    float minP = 0.05f;
    float temperature = 0.8f;
    uint32_t seed = 0xFFFFFFFF;
};

struct TextGenOptions : TextGenOptionsBase
{
    TokenCallback onToken = nullptr;
    ProgressCallback onInputEval = nullptr;
};

struct AsyncTextGenOptions : TextGenOptions {
    FinishCallback onDone = nullptr;
};
