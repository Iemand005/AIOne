#pragma once

#include <string>
#include <iostream>
#include <functional>
#include <cstdint>
#include <stdexcept>
#include <algorithm>
#include <memory>
#include <vector>

// #include <llama-cpp.h>
// #include <ggml-backend.h>
// #include <llama-context.h>

#include "AIOneAPI.hpp"
#include "TextContextOptions.h"
#include "ContextInvalidError.hpp"

class LLModel;
typedef int32_t llama_token;
struct llama_context;
struct llama_chat_message;
typedef int32_t llama_seq_id;

class AIONE_API TextContext
{
    LLModel *modelWrapper;

    // llama_context *context; // Kept alive by LLModel
    // llama_seq_id seqId;

    struct Impl;
    std::unique_ptr<Impl> impl;



    void throwIfFreed();

public:
    TextContext(LLModel *modelWrapper, llama_context *context);
    TextContext(const TextContext&) = delete;
    TextContext& operator=(const TextContext&) = delete;
    TextContext(TextContext&& other) noexcept;
    TextContext& operator=(TextContext&& other) noexcept;

    bool operator==(const TextContext& other) const ;

    bool operator!=(const TextContext& other) const {
        return !(*this == other);
    }

    bool isValid();

    bool isConnectedTo(LLModel *model);

    llama_context *getContext();

    llama_seq_id getSeqId() ;

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

    ~TextContext();
};
