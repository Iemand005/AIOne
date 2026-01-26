#pragma once
#include <llama-cpp.h>
#include <string>

#include "llmodel.hpp"
#include "sdmodel.hpp"

class ModelFactory {
    bool initializedLlama = false;
public:
    void initLlama() {
        if (initializedLlama) return;
        // llama_backend_init();
        // ggml_backend_load_all();
        initializedLlama = true;
    }

    std::unique_ptr<LLModel> loadLLM(std::string path) {
        initLlama();
        return std::make_unique<LLModel>(path);
    }

    std::unique_ptr<SDModel> loadSDM(std::string path) {
        return std::make_unique<SDModel>(path);
    }

    const char *systemInfoStr() {
        return llama_print_system_info();
    }

};
