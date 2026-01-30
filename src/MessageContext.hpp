#pragma once

#include <string>
#include <iostream>
#include <functional>
#include <cstdint>
#include <stdexcept>
#include <algorithm>

// #include <llama-cpp.h>
// // #include <src/llama-context.h>
// #include <ggml-backend.h>

// #include <llama-context.h>

// #include "LLModel.hpp"

#include "MessageContextOptions.hpp"
#include "ContextInvalidError.hpp"

class LLModel;
typedef int32_t llama_token;
struct llama_context;
struct llama_chat_message;
typedef int32_t llama_seq_id;

class MessageContext
{
    LLModel *modelWrapper;
    llama_context *context; // Kept alive by LLModel

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
    MessageContext(LLModel *modelWrapper, llama_context *context);

    bool operator==(const MessageContext& other) const {
        return this->seqId == other.seqId;
    }

    bool operator!=(const MessageContext& other) const {
        return !(*this == other);
    }

    bool isValid();

    bool isConnectedTo(LLModel *model);

    llama_context *getContext() {
        return context;
    }

    llama_seq_id getSeqId() {
        return seqId;
    }

    uint32_t getContextLength() ;

    uint32_t getUsedContextLength() ;

    uint32_t getLastIndex() ;

    uint32_t getBatchSize() ;

    /**
     * Adds the tokens to the cache, truncates cache misses from
     * the underlying context, and returns the index of the next
     * first token that had a cache miss.
     * 
     * For llama_encode and llama_decode, this means you should start
     * encoding/decoding from that index forward.
     */
    size_t addCache(const std::vector<llama_token> &tokens) ;

    /**
     * Adds the tokens to the cache, truncates cache misses from
     * the underlying context, and returns the index of the next
     * first token that had a cache miss.
     * 
     * For llama_encode and llama_decode, this means you should start
     * encoding/decoding from that index forward.
     */
    void addCache(const std::vector<llama_token> &tokens, size_t cacheMissIndex) ;
    
    size_t findCache(const std::vector<llama_token> &tokens) ;



    void destroy();

    ~MessageContext() {
        destroy();
    }
};
