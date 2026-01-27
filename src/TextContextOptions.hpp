#pragma once

#include <cstdint>

struct TextContextOptions
{
    uint32_t contextLength = 4096;
    uint32_t evalBatchSize = 512;
};
