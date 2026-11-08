#pragma once
#include "AIOneAPI.hpp"
#include "Model.hpp"
#include "ModelOptions.h"
#include "TTSModelOptions.h"
#include "TTSSynthesisOptions.hpp"
#include "TTSSynthesisResult.hpp"
 
#include <vector>
#include <string>
#include <iostream>
#include <llama.h>
#include <llama-model.h>
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
    TTSModel(const std::string& path, bool load = true);
    TTSModel(const std::string& path, TTSModelOptions options, bool load = true);
    TTSModel(const std::string& path, const std::string& mmprojPath, bool load = true);
    TTSModel(const std::string& path, TTSModelOptions options, const std::string& mmprojPath, ModelOptions mmprojOptions, bool load = true);
    ~TTSModel();

    bool loadModel();
    bool loadModel(const std::string& path, TTSModelOptions options = {});
    bool loadMmproj();
    bool loadMmproj(const std::string& path, ModelOptions options = {});
    
    std::vector<std::string> getVoices();
    bool saveAudioAsWAV(const TTSAudio& audio, const std::string& filename);

    TTSSynthesisResult synthesize(const std::string& prompt, TTSSynthesisOptions options = {});

    bool isModelLoaded() const { return loaded; }
    bool isMmprojLoaded() const { return loadedMmproj; }
    bool isReady() const { return loaded && loadedMmproj; }

private:
    std::string modelPath = "";
    std::string mmprojPath = "";
    TTSModelOptions modelOptions = {};
    ModelOptions mmprojOptions = {};

    bool loaded = false;
    bool loadedMmproj = false;
    common_init_result_ptr llamaInit;
    llama_model* model;
    llama_context* ctx;
    common_sampler* sampler;
    mtmd::context_ptr mctx;
    mtmd_gen_audio_info mctxInfo;

    bool requireFull(bool throwQ = false) {
        if (loaded && loadedMmproj) {
            return true;
        }
        
        if (throwQ) {
            throw std::runtime_error("Backbone model and/or mmproj not loaded, call loadModel(...) and loadMmproj(...) first");
        } else {
            std::cerr << "Backbone model and/or mmproj not loaded, call loadModel(...) and loadMmproj(...) first" << std::endl;
        }

        return false;
    }
};

typedef std::unique_ptr<TTSModel> TTSModelPtr;