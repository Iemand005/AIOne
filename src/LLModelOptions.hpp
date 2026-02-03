#pragma once

#include <cstdint>

#include "ModelOptions.h"
#include "PreferredDevice.h"
#include "TextContextOptions.h"


struct LLModelOptions : ModelOptions, TextContextOptions
{
    PreferredDevice device = PreferredDevice::ANY;
    int32_t offloadLayers = -1; // -1 for all layers
};
