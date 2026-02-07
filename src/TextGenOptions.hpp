#pragma once

#include <cstdint>
#include <functional>

#include "TextGenResult.hpp"
#include "Callbacks.h"

typedef std::function<void(const std::string &token)> TokenCallback;
typedef std::function<void(const std::string &token, bool thinking)> TokenReasoningCallback;
typedef std::function<void(const TextGenResult &output)> TextFinishCallback;
typedef std::function<void()> ThinkStartCallback;
typedef std::function<void()> ThinkEndCallback;
typedef std::function<void(bool thinking)> ThinkStateChangedCallback;

struct TextGenOptionsBase
{
    size_t maxTokens = 400;
    float minP = 0.05f;
    float temperature = 0.8f;
    uint32_t seed = 0xFFFFFFFF;
};

// struct TextGenCallbacks {

// }

struct TextGenOptions : TextGenOptionsBase
{
    TokenCallback onToken = nullptr;
    ProgressCallback onInputEval = nullptr;

    TokenReasoningCallback onTokenReasoning;
    ThinkStateChangedCallback onThinkStateChange;

    std::string systemPrompt = "";
};

struct AsyncTextGenOptions : TextGenOptions {
    TextFinishCallback onDone = nullptr;
};
