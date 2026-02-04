#pragma once

#include <string>

#include <llama-cpp.h>

#include "LLModel.hpp"
#include "SDModel.hpp"





class ModelFactory {
    bool loadedBackends = false;
    bool initializedLlama = false;
public:
    void initLlama();
    void loadBackends();

    std::unique_ptr<LLModel> loadLLM(const std::string path) {
        initLlama();
        auto model = std::make_unique<LLModel>(path);
        return model;
    }

    std::unique_ptr<SDModel> loadSDM(const std::string path) {
        auto model = std::make_unique<SDModel>(path);
        return model;
    }

    void convertSDModelAsync(std::string source, QuantTypes level, std::string destination, ProgressCallback callback) {
        std::thread([this, source, level, destination, callback]() {
            convertSDModel(source, level, destination, callback);
        }).detach();
    }

    bool convertSDModel(std::string source, QuantTypes level, std::string destination, ProgressCallback callback) {
        auto model = new SDModel(source, false);
        model->setProgressCallback(callback);
        bool success = model->exportToGGUF(destination, level, false);
        return success;
    }

    const char *systemInfoStr() ;

};
