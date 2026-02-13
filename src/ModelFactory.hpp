#pragma once

#include <string>
#include <future>

// #include <llama-cpp.h>

#include "AIOneAPI.hpp"
#include "LLModel.hpp"
#include "SDModel.hpp"
#include "Callbacks.h"

// template<typename T>
// using FinishedCallback = std::function<void(T)>;

typedef FinishedTCallback<LLModelPtr> LoadLLModelFinished;
typedef FinishedTCallback<SDModelPtr> LoadSDModelFinished;

class AIONE_API ModelFactory {
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

    void loadLLMAsync(const std::string path, LLModelOptions options = {}, LoadLLModelFinished onDone = nullptr) {
        runAsync([this, path, options, onDone]() { onDone(loadLLM(path, options)); });
    }

    SDModelPtr loadSDM(const std::string path) {
        return std::make_unique<SDModel>(path);
    }

    void convertSDModelAsync(std::string source, QuantTypes level, std::string destination, ProgressCallback callback, FinishedTCallback<bool> onDone = nullptr) {
        runAsync([this, source, level, destination, callback, onDone]() {
            onDone(convertSDModel(source, level, destination, callback));
        });
    }

    bool convertSDModel(std::string source, QuantTypes level, std::string destination, ProgressCallback callback, FinishedTCallback<bool> onDone = nullptr) {
        auto model = new SDModel(source, false);
        model->setProgressCallback(callback);
        bool success = model->exportToGGUF(destination, level, false);
        return success;
    }

    const char *systemInfoStr();

};

typedef std::unique_ptr<ModelFactory> ModelFactoryPtr;
