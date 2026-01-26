#pragma once

#include <string>
#include <ggml.h>

class Model {
public:
    bool loadModel(const std::string& path);

    ggml_backend_device *getBackend();
};
