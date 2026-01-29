#pragma once

#include <string>

#include <llama-cpp.h>

#include "LLModel.hpp"
#include "SDModel.hpp"

class ModelFactory {
    bool initializedLlama = false;
public:
    void initLlama() {
        if (initializedLlama) return;
        llama_backend_init();
        ggml_backend_load_all();
        initializedLlama = true;
    }

    std::unique_ptr<LLModel> loadLLM(const std::string &path) {
        initLlama();
        auto model = std::make_unique<LLModel>(path);
        // model->loadModel(path);
        return model;
    }

    std::unique_ptr<SDModel> loadSDM(const std::string &path) {
        auto model = std::make_unique<SDModel>();
        model->loadModel(path);
        return model;
    }

    const char *systemInfoStr() {
        return llama_print_system_info();
    }

};
