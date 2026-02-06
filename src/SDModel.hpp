#pragma once
#include <string>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <vector>
#include <functional>
#include <thread>
#include <mutex >
#include <queue>


// #include <ggml-backend.h>
// #include <stable-diffusion.h>

#include "SDImageOptions.h"
#include "SDModelOptions.hpp"
#include "Callbacks.h"
#include "Random.h"

#include "Model.hpp"

inline void setEnvironmentVariable(const std::string& name, const std::string& value) {
#ifdef _WIN32
    _putenv_s(name.c_str(), value.c_str());
#else
    setenv(name.c_str(), value.c_str(), 1);
#endif
}

// Identical to sd_image_t
struct SDImage {
    uint32_t width;
    uint32_t height;
    uint32_t channel;
    uint8_t* data;
} ;

typedef std::function<void(int step, SDImage* image, bool isNoisy)> PreviewCallback;

enum QuantTypes : int {
    Q4_0 = 0,
    Q4_1 = 1,
    Q5_0 = 2,
    Q5_1 = 3,
    Q8_0 = 4,
    F16 = 5,
    F32 = 6
};



class SDModel : public Model {
    std::string lastPrompt;

    std::string modelPath = "";
    SDModelOptions options = {};

public:
// struct Impl;
// 
private:

struct Impl;
    std::unique_ptr<Impl> impl;
    
    struct SafeImage {
        std::vector<uint8_t> data;
        int width = 0;
        int height = 0;
        int channel = 0;
    };
    SafeImage lastResult;

    // ProgressCallback progressCallback = nullptr;
    PreviewCallback previewCallback = nullptr;

public:
    SDModel();
    SDModel(std::string path, bool load = true)  ;
    SDModel(std::string path, SDModelOptions options, bool load = true) : SDModel(path) {
        this->options = options;
        if (load) loadModel();
    }

    ~SDModel();

    void freeContext() ;;

    void selectDevice(int device = 1) {
        setEnvironmentVariable("SD_VK_DEVICE", std::to_string(device));
    }

    void setAllowSharedMemory(bool allow = true) {
        setEnvironmentVariable("GGML_VK_ALLOW_SYSMEM_FALLBACK", std::to_string(allow));
    }

    bool loadModel() {
        return loadModel(modelPath, options);
    }

    bool loadModel(const std::string path, SDModelOptions options = {});



    bool exportToGGUF(std::string destination, QuantTypes level = Q4_0, bool convertTensorsName = false) ;

    bool saveImageAsPNG(const SDImage& image, const std::string& filename) ;



    void setProgressCallback(ProgressCallback callback) ;

    void setPreviewCallback(PreviewCallback callback, SDPreviewMode mode = Proj) ;

    void clearProgressCallback() ;
    void clearPreviewCallback() ;

    

    std::thread generationWorker;

    std::vector<std::thread> generationThreads;
    std::queue<std::function<void()>> tasks;
    std::mutex queueMutex;
    std::thread generationThread;

    using ImageCompleteHandler = std::function<void(SDImage image)>;
    
        uint64_t newSeed() {
            return Random::int64();
        }

    void generateAsync(const std::string positive, const std::string negative = "", SDImageOptions options = SDImageOptions{}, ImageCompleteHandler callback = nullptr) {
        std::thread([this, positive, negative, options, callback]() {
            try {
                auto image = generateImage(positive, negative, options);
                if (callback) callback(image);
            } catch (std::exception ex) {
                std::cerr << "Image generation failed because: " << ex.what() << std::endl;
            }
        }).detach();
    }

    SDImage generateImage(const std::string positive, const std::string negative = "", SDImageOptions options = SDImageOptions{}) ;
};

typedef std::unique_ptr<SDModel> SDModelPtr;
