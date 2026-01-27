#pragma once

#include <string>
#include <iostream>
#include <functional>
#include <cstdint>
#include <stdexcept>

#include <llama-cpp.h>
#include <llama-context.h>
#include <ggml-backend.h>

#include "LLModel.hpp"
#include "TextContextOptions.hpp"

class TextContext
{
    llama_model *model;
    llama_context *context;
    std::vector<llama_chat_message> messages;

    void throwIfFreed() {
        if (context == NULL) {
            throw std::runtime_error("Context is freed");
        }
    }

public:
    TextContext(llama_model *model, const TextContextOptions &options) {
        this->model = model;

        llama_context_params contextParams = llama_context_default_params();
        contextParams.n_ctx = options.contextLength;
        contextParams.n_batch = options.evalBatchSize;

        context = llama_init_from_model(model, contextParams);
        if (context == NULL) {
            throw std::runtime_error("");
        }
    }

    uint32_t getContextLength() {
        throwIfFreed();
        return llama_n_ctx(context);
    }

    uint32_t getUsedContextLength() {
        throwIfFreed();
        
        // gets the last index in the kv cache (sequence 0), and adds 1 to get the length
        return llama_memory_seq_pos_max(llama_get_memory(context), 0) + 1;
    }

    uint32_t getLastIndex() {
        throwIfFreed();
        
        // gets the last index in the kv cache (sequence 0)
        return llama_memory_seq_pos_max(llama_get_memory(context), 0);
    }

    uint32_t getBatchSize() {
        return llama_n_batch(context);
    }

    llama_context* getRawContext() {
        return context;
    }



    void destroy()
    {
        if (context != NULL) {
            llama_free(context);
            context = NULL;
        }
    }

    ~TextContext()
    {
        destroy();
        // Smart pointers handle cleanup automatically
    }
};
