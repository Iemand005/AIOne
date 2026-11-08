#pragma once

#include <string>
#include <future>

// #include <llama-cpp.h>

#include "AIOneAPI.hpp"
#include "LLModel.hpp"
#include "SDModel.hpp"
#include "TTSModel.hpp"
#include "Callbacks.h"
#include "ModelOptions.h"

// template<typename T>
// using FinishedCallback = std::function<void(T)>;

typedef FinishedTCallback<LLModelPtr> LoadLLModelFinished;
typedef FinishedTCallback<SDModelPtr> LoadSDModelFinished;
typedef FinishedTCallback<TTSModelPtr> LoadTTSModelFinished;

class AIONE_API ModelFactory {
    bool loadedBackends = false;
    bool initializedLlama = false;
    bool initializedCommon = false;
public:
    void initLlama();
    void initCommon();
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


    SDModelPtr loadSDM(const std::string path, SDModelOptions options) {
        return std::make_unique<SDModel>(path, options, true);
    }

    void loadSDMAsync(const std::string path, SDModelOptions options = {}, LoadSDModelFinished onDone = nullptr) {
        runAsync([this, path, options, onDone]() { onDone(loadSDM(path, options)); });
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


    TTSModelPtr loadTTSM(const std::string path, ModelOptions options, const std::string mmprojPath, ModelOptions mmprojOptions) {
        return std::make_unique<TTSModel>(path, options, mmprojPath, mmprojOptions, true);
    }

    void loadTTSMAsync(const std::string path, const std::string mmprojPath, LoadTTSModelFinished onDone = nullptr) {
        loadTTSMAsync(path, {}, mmprojPath, {}, onDone);
    }

    void loadTTSMAsync(const std::string path, ModelOptions options, const std::string mmprojPath, ModelOptions mmprojOptions = {}, LoadTTSModelFinished onDone = nullptr) {
        runAsync([this, path, options, mmprojPath, mmprojOptions, onDone]() {
            onDone(loadTTSM(path, options, mmprojPath, mmprojOptions));
        });
    }


    const char *systemInfoStr();
};

typedef std::unique_ptr<ModelFactory> ModelFactoryPtr;
