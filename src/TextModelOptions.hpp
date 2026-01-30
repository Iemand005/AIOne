#pragma once

#include <cstdint>

#include "PreferredDevice.hpp"
#include "MessageContextOptions.h"

struct TextModelOptions : MessageContextOptions
{
    PreferredDevice device = PreferredDevice::ANY;
    int32_t offloadLayers = -1; // -1 for all layers
};
