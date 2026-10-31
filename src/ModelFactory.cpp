
#include "ModelFactory.hpp"
#include <llama-cpp.h>

#ifndef AIONE_VERBOSE
static void llamaLogger(enum ggml_log_level level, const char *text, void *user_data) {}
#endif

void ModelFactory::loadBackends() {
    if (loadedBackends) return;
#ifndef AIONE_VERBOSE
    llama_log_set(llamaLogger, this);
#endif
    ggml_backend_load_all();
    loadedBackends = true;
}

void ModelFactory::initLlama() {
    if (initializedLlama) return;
#ifndef AIONE_VERBOSE
    llama_log_set(llamaLogger, this);
#endif
    llama_backend_init();
    initializedLlama = true;
}

const char *ModelFactory::systemInfoStr()  {
    return llama_print_system_info();
}
