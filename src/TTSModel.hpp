#pragma once
#include "AIOneAPI.hpp"
#include "Model.hpp"

class AIONE_API TTSModel : public Model {
public:
    ~TTSModel();

    bool loadModel(const std::string& path);
    bool loadMmproj(const std::string& path);

private:
    bool loaded = false;
    llama_model* model;
    llama_context* ctx;
    common_sampler* sampler;
};