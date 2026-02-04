#pragma once

#include <string>
#include <vector>

#include "PreferredDevice.h"
#include "Callbacks.h"

struct ggml_backend_device;
typedef ggml_backend_device * ggml_backend_dev_t;

// This is for the shared GGML stuff
class Model {
    ProgressCallback onProgress;
public:
    Model *super() {
        return this;
    }

    std::vector<ggml_backend_dev_t> devices = std::vector<ggml_backend_dev_t>(2);

    bool loadModel(const std::string& path);
    ggml_backend_device *getBackend();
    void selectDevice(PreferredDevice preferred);
    void setProgressCallback(ProgressCallback callback);
    ProgressCallback progressCallback();
    void clearProgressCallback() {
        onProgress = nullptr;
    }
};
