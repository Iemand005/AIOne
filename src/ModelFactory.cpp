
#include "ModelFactory.hpp"

void ModelFactory::loadBackends() {
    if (loadedBackends) return;
    ggml_backend_load_all();
    loadedBackends = true;
}

void ModelFactory::initLlama() {
    if (initializedLlama) return;
    llama_backend_init();
    initializedLlama = true;
}

const char *ModelFactory::systemInfoStr()  {
    return llama_print_system_info();
}
