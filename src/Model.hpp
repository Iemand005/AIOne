#pragma once

#include <string>
#include <ggml.h>

class Model {
public:
    virtual bool loadModel(const std::string& path) {};

    virtual ggml_backend_device *getBackend() {};
};
