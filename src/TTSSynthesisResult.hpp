#pragma once
#include "TTSAudio.hpp"

struct TTSSynthesisResult {
    int32_t sampleRate = 0;
    int64_t samples = 0;
    TTSAudio audio;
    
    double promptEvalMs;
    double generationMs;
    double conversionMs;
};