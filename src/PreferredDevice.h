#pragma once

#include <cstdint>

enum PreferredDevice : uint8_t
{
    ANY,
    CPU,
    IGPU,
    DGPU,
    ACCELERATOR
};
