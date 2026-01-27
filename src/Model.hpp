#pragma once

#include <string>
#include <ggml.h>

class Model {
public:
    virtual bool loadModel(const std::string& path) = 0;

    virtual ggml_backend_device *getBackend() = 0;
};
