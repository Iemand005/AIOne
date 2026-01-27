#pragma once

#include <cstdint>

enum PreferredDevice : uint8_t
{
    ANY,
    DGPU,
    IGPU,
    CPU
};
