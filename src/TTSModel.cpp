#include "TTSModel.hpp"
#include <iostream>
#include <common.h>
#include <sampling.h>
#include <mtmd.h>

bool TTSModel::loadModel(const std::string& path) {
    if (loaded) {
        return true;
    }

    // TODO verbosity: 
    //mtmd_helper_log_set(common_log_default_callback, nullptr);

    // init common + llama.cpp
    common_init();

    if (ggml_backend_reg_count() == 0) { // only load if not initialized yet
        llama_backend_init();
    }

    // load model
    common_params params;
    params.model.path = path;
    params.embedding = true; // required to pass hidden states to audio gen helper

    common_init_result_ptr llamaInit = common_init_from_params(params);
    model = llamaInit->model();
    if (!model) {
        std::cerr << "Failed to initialize TTS model" << std::endl;
        return false;
    }

    ctx = llamaInit->context();
    if (!ctx) {
        std::cerr << "Failed to initialize TTS context" << std::endl;
        return false;
    }

    sampler = llamaInit->sampler(0);
    return true;
}


bool TTSModel::loadMmproj(const std::string& path/*, options // TODO*/) {
    if (!loaded) {
        std::cerr << "Backbone model not loaded, call loadModel(...) first" << std::endl;
        return false;
    }

/*
    // TODO

    mtmd_context_params mtmdParams = mtmd_context_params_default();
    mtmdParams.device = getBackend();

    enum ggml_backend_dev_type deviceType = ggml_backend_dev_type(mtmdParams.device);
    mtmdParams.use_gpu = (deviceType == GGML_BACKEND_DEVICE_TYPE_GPU || deviceType == GGML_BACKEND_DEVICE_TYPE_IGPU);
    mtmdParams.flash_attn_type = // TODO
    mtmdParams.n_threads = // TODO
    
    mtmd::context_ptr mctx(mtmd_init_from_file(...)) // TODO
*/
    return true;
}

TTSModel::~TTSModel() {
    if (sampler != nullptr) {
        common_sampler_free(sampler);
    }

    if (ctx != nullptr) {
        llama_free(ctx);
    }

    if (model != nullptr) {
        llama_free_model(model);
    }

    loaded = false;
}