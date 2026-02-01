#pragma once

#include <cstdint>

#include "PreferredDevice.h"
#include "TextContextOptions.h"

struct TextModelOptions : TextContextOptions
{
    PreferredDevice device = PreferredDevice::ANY;
    int32_t offloadLayers = -1; // -1 for all layers
};
