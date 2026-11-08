#pragma once
#include <vector>

enum TTSAudioType {
    PCM, // raw PCM
    WAV, // WAV PCM 16-bit LE, mono
};

struct TTSAudio {
    TTSAudioType type;
    std::vector<char> data;
};