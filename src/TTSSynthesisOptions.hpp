#pragma once
#include <string>

#ifdef AIONE_ENABLE_TTS
#include <mtmd-helper.h>
#else
namespace mtmd_helper {
    struct gen_audio {};
}
#endif

#include "Callbacks.h"
#include "TTSAudio.hpp"

typedef std::function<void(const int frameIdx, const mtmd_helper::gen_audio &gen)> FrameCallback;

struct TTSVoice {
    std::string referenceFile = "";
    std::string builtInName = "";
    int32_t builtInIndex = 0;
};

struct TTSSynthesisOptions
{
    TTSAudioType outputType;
    TTSVoice voice;
    std::string lang;
    short topK = 40; // TODO find defaults
    float topP = 0.05f; // TODO find defaults
    uint32_t seed = 0xFFFFFFFF;
    int32_t batchSize = 512;
    int maxTokens = -1;
    
    FrameCallback onFrame = nullptr;
};

struct TTSSynthesisOptionsAsync : CallbackAsyncBase, TTSSynthesisOptions
{
    // FinishedCallback onDone = nullptr;
};