#pragma once

#include "Callbacks.h"
#include "ModelOptions.h"
#include "PreferredDevice.h"

struct TTSModelOptions : ModelOptions {
    PreferredDevice device = PreferredDevice::ANY;
};

struct TTSModelOptionsAsync : CallbackAsyncBase, TTSModelOptions
{
    // FinishedCallback onDone = nullptr;
};