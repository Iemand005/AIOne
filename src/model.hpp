#pragma once

#include <llama-cpp.h>

class Model {
public:
    Model() {}
    void init() {
        llama_backend_init();
        ggml_backend_load_all();
    }
};
