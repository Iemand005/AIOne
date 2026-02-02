#pragma once

#include <cstdint>

// #include <llama-cpp.h>

struct TextGenOptions
{
    float minP = 0.05f;
    float temperature = 0.8f;
    uint32_t seed = 0xFFFFFFFF;
};
