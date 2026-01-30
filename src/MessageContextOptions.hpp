#pragma once

#include <cstdint>

struct MessageContextOptions
{
    uint32_t contextLength = 4096;
    uint32_t evalBatchSize = 512;
    uint32_t threadCount = 6;
};
