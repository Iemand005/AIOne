#pragma once
#include "AIOneAPI.hpp"
#include "Model.hpp"
#include "ModelOptions.h"
#include "TTSSynthesisOptions.hpp"
#include "TTSSynthesisResult.hpp"
 
#include <vector>
#include <string>
#include <iostream>
#include <llama.h>
#include <common.h>

#ifdef AIONE_ENABLE_TTS
#include <mtmd.h>
#else
namespace mtmd {
    struct context {};
    using context_ptr = context*;
}

struct mtmd_gen_audio_info {};
#endif

class AIONE_API TTSModel : public Model {
public:
    ~TTSModel();

    bool loadModel(const std::string& path, ModelOptions options = {});
    bool loadMmproj(const std::string& path, ModelOptions options = {});
    
    std::vector<std::string> getVoices();
    bool saveAudioAsWAV(const TTSAudio& audio, const std::string& filename);

    TTSSynthesisResult synthesize(const std::string& prompt, TTSSynthesisOptions options = {});

private:
    bool loaded = false;
    bool loadedMmproj = false;
    llama_model* model;
    llama_context* ctx;
    common_sampler* sampler;
    mtmd::context_ptr mctx;
    mtmd_gen_audio_info mctxInfo;

    bool requireFull() {
        if (loaded && loadedMmproj) {
            return true;
        }
        
        std::cerr << "Backbone model and/or mmproj not loaded, call loadModel(...) and loadMmproj(...) first" << std::endl;
        return false;
    }
};