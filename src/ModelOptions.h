#pragma once

#include "Callbacks.h"

struct ModelOptions {
    short threadCount = 6;
    bool flashAttention = true;

    ProgressCallback onProgress;
};

struct ModelOptionsAsync : CallbackAsyncBase, ModelOptions
{
    // FinishedCallback onDone = nullptr;
};