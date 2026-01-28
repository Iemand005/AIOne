#pragma once

#include <string>
#include <iostream>
#include <functional>
#include <cstdint>
#include <stdexcept>
#include <algorithm>

#include <llama-cpp.h>
// #include <src/llama-context.h>
#include <ggml-backend.h>

#include "LLModel.hpp"
#include "TextContextOptions.hpp"
#include "ContextInvalidError.hpp"

class TextContext
{
    LLModel *modelWrapper;
    llama_model *const model;
    llama_context *context;
    std::vector<llama_chat_message> messages;
    std::vector<llama_token> cache;
    llama_seq_id seqId;

    void throwIfFreed() {
        if (context == nullptr) {
            throw std::runtime_error("This context is freed");
        }

        if (!isValid()) {
            throw ContextInvalidError("Model context was reset; this context is invalid");
        }
    }

public:
    TextContext(LLModel *modelWrapper, llama_model *model, llama_context *context) : modelWrapper(modelWrapper), model(model), context(context) {
        this->modelWrapper = modelWrapper;
        seqId = modelWrapper->claimSeqId();
    }

    bool isValid() {
        return modelWrapper->isValid(context);
    }

    bool isConnectedTo(LLModel *model) {
        return modelWrapper == model;
    }

    llama_seq_id getSeqId() {
        return seqId;
    }

    uint32_t getContextLength() {
        throwIfFreed();
        return llama_n_ctx(context);
    }

    uint32_t getUsedContextLength() {
        throwIfFreed();
        
        // gets the last index in the kv cache, and adds 1 to get the length
        return llama_memory_seq_pos_max(llama_get_memory(context), seqId) + 1;
    }

    uint32_t getLastIndex() {
        throwIfFreed();
        
        // gets the last index in the kv cache
        return llama_memory_seq_pos_max(llama_get_memory(context), seqId);
    }

    uint32_t getBatchSize() {
        return llama_n_batch(context);
    }

    /**
     * Adds the tokens to the cache, truncates cache misses from
     * the underlying context, and returns the index of the next
     * first token that had a cache miss.
     * 
     * For llama_encode and llama_decode, this means you should start
     * encoding/decoding from that index forward.
     */
    size_t addCache(const std::vector<llama_token> &tokens) {
        size_t cacheMissIndex = findCache(tokens);
        addCache(tokens, cacheMissIndex);

        return cacheMissIndex;
    }

    /**
     * Adds the tokens to the cache, truncates cache misses from
     * the underlying context, and returns the index of the next
     * first token that had a cache miss.
     * 
     * For llama_encode and llama_decode, this means you should start
     * encoding/decoding from that index forward.
     */
    void addCache(const std::vector<llama_token> &tokens, size_t cacheMissIndex) {
        // truncate cached context starting from `cacheMissIndex`
        // (if it's after the cached context size, skip truncation because whole cached prompt matched)
        if (cacheMissIndex < cache.size()) {
            llama_memory_seq_rm(llama_get_memory(context), seqId, cacheMissIndex, -1);
        }

        // copy only new tokens into the cache
        cache.resize(tokens.size());
        std::copy(tokens.begin() + cacheMissIndex, tokens.end(), cache.begin() + cacheMissIndex);
    }
    
    size_t findCache(const std::vector<llama_token> &tokens) {
        size_t searchLength = std::min(tokens.size(), cache.size());

        // loop through the cache until `tokens[i]` does not match `cache[i]` anymore
        size_t cacheMissIndex = 0;
        while (cacheMissIndex < searchLength && tokens[cacheMissIndex] == cache[cacheMissIndex]) {
            cacheMissIndex++;
        }

        return cacheMissIndex;
    }

    void destroy() {
        if (context != nullptr) {
            llama_free(context);
            modelWrapper->releaseSeqId(seqId);

            context = nullptr;
        }
    }

    ~TextContext() {
        destroy();
        // Smart pointers handle cleanup automatically
    }
};
