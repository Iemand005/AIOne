#pragma once

#include <llama-cpp.h>
#include <string>

class Model {
public:
    Model() {
        llama_backend_init();
        ggml_backend_load_all();
    }
    Model(std::string path) {
        llama_model_params model_params = llama_model_default_params();
        llama_model_ptr model(llama_model_load_from_file(path.c_str(), model_params));
        this->model = std::move(model);
    }
};
