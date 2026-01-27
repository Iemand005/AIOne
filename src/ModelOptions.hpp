#pragma once

#include <cstdint>

#include "PreferredDevice.hpp"

struct ModelOptions
{
    PreferredDevice device = PreferredDevice::ANY;
    int32_t offloadLayers = 0; // -1 for all layers
};
