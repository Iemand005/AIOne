#pragma once

#include <cstdint>

#include "ModelOptions.h"
#include "PreferredDevice.h"
#include "TextContextOptions.h"
#include "Callbacks.h"

struct LLModelOptions : ModelOptions, TextContextOptions
{
    PreferredDevice device = PreferredDevice::ANY;
    int32_t offloadLayers = -1; // -1 for all layers
    bool useMmap = false;
};

struct LLModelOptionsAsync : CallbackAsyncBase, LLModelOptions
{
    // FinishedCallback onDone = nullptr;
};
