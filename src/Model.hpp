#pragma once

#include <string>

#include "PreferredDevice.h"

class Model {
    std::vector<ggml_backend_dev_t> devices = {nullptr, nullptr};
public:

    bool loadModel(const std::string& path);
    ggml_backend_device *getBackend();
    void selectDevice(PreferredDevice preferred);
};
