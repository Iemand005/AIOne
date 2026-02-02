#pragma once

#include <cstdint>
#include <string>
#include <functional>

#include "PreferredDevice.h"
#include "TextContextOptions.h"
#include "TextGenerationStats.hpp"

typedef std::function<void(const float progress)> ProgressCallback;
typedef std::function<void(const std::string &token)> TokenCallback;
typedef std::function<void(const TextGenResult &output)> FinishCallback;

struct LLModelOptions : TextContextOptions
{
    PreferredDevice device = PreferredDevice::ANY;
    int32_t offloadLayers = -1; // -1 for all layers
};
