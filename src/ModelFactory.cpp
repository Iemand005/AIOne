
#include "ModelFactory.hpp"
#include <llama-cpp.h>
#include <common.h>
#include <log.h>

#ifndef AIONE_VERBOSE
#ifdef AIONE_ENABLE_TTS
#include <mtmd-helper.h>
#endif // AIONE_ENABLE_TTS

static void llamaLogger(enum ggml_log_level level, const char *text, void *user_data) {}
#endif // AIONE_VERBOSE

void ModelFactory::loadBackends() {
    if (loadedBackends) return;
#ifndef AIONE_VERBOSE
    llama_log_set(llamaLogger, nullptr);
#endif
    ggml_backend_load_all();
    loadedBackends = true;
}

void ModelFactory::initLlama() {
    if (initializedLlama) return;
#ifndef AIONE_VERBOSE
    llama_log_set(llamaLogger, nullptr);
#endif
    llama_backend_init();
    initializedLlama = true;
}

void ModelFactory::initCommon() {
    if (initializedCommon) return;
#ifndef AIONE_VERBOSE
    common_log_set_verbosity_thold(LOG_LEVEL_OUTPUT);
#ifdef AIONE_ENABLE_TTS
    mtmd_helper_log_set(llamaLogger, nullptr);
#endif // AIONE_ENABLE_TTS
#endif // AIONE_VERBOSE
    common_init();
    initializedCommon = true;
}

const char *ModelFactory::systemInfoStr()  {
    return llama_print_system_info();
}
