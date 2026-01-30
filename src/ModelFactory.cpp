
#include <llama-cpp.h>

#include "ModelFactory.hpp"

void ModelFactory::initLlama() {
        if (initializedLlama) return;
        llama_backend_init();
        ggml_backend_load_all();
        initializedLlama = true;
    }

   const char *ModelFactory::systemInfoStr()  {
        return llama_print_system_info();
    }