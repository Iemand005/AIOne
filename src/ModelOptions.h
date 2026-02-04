#pragma once

#include "Callbacks.h"

struct ModelOptions {
    short threadCount = 6;
    bool flashAttention = true;

    ProgressCallback onProgress;
};
