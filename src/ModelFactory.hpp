#pragma once

#include <string>

#include <llama-cpp.h>

#include "LLModel.hpp"
#include "SDModel.hpp"

template<typename T>
using FinishedCallback = std::function<void(T)>;

typedef FinishedCallback<LLModelPtr> LoadLLModelFinished;
typedef FinishedCallback<SDModelPtr> LoadSDModelFinished;

class ModelFactory {
    bool loadedBackends = false;
    bool initializedLlama = false;
public:
    void initLlama();
    void loadBackends();

    void runAsync(std::function<void()> func) {
        std::thread(func).detach();
    }

    LLModelPtr loadLLM(const std::string path, LLModelOptions options) {
        initLlama();
        return std::make_unique<LLModel>(path, options);
    }

    void loadLLMAsync(const std::string path, LLModelOptions options = {}, LoadLLModelFinished onDone = nullptr, ProgressCallback onProgress = nullptr) {
        options.onProgress = onProgress;
        runAsync([this, path, options, onDone]() { onDone(loadLLM(path, options)); });
    }

    SDModelPtr loadSDM(const std::string path) {
        return std::make_unique<SDModel>(path);
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

    const char *systemInfoStr();

};
