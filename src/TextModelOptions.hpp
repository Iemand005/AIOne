#pragma once

#include <cstdint>

#include "PreferredDevice.hpp"
#include "TextContextOptions.hpp"

struct TextModelOptions : TextContextOptions
{
    PreferredDevice device = PreferredDevice::ANY;
    int32_t offloadLayers = -1; // -1 for all layers
};
