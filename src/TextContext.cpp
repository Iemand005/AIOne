

#include "TextContext.hpp"
#include "LLModel.hpp"

TextContext::TextContext(LLModel *modelWrapper, llama_context *context) : modelWrapper(modelWrapper), context(context) {
    seqId = modelWrapper->claimSeqId();
}

bool TextContext::isValid() {
    return modelWrapper->isValid(context);
}

bool TextContext::isConnectedTo(LLModel *model) {
    return modelWrapper == model;
}

void TextContext::destroy() {
}


uint32_t TextContext::getContextLength() {
    throwIfFreed();
    return llama_n_ctx(context);
}

uint32_t TextContext::getUsedContextLength() {
    throwIfFreed();
    
    // gets the last index in the kv cache, and adds 1 to get the length
    return llama_memory_seq_pos_max(llama_get_memory(context), seqId) + 1;
}

uint32_t TextContext::getLastIndex() {
    throwIfFreed();
    
    // gets the last index in the kv cache
    return llama_memory_seq_pos_max(llama_get_memory(context), seqId);
}

uint32_t TextContext::getBatchSize() {
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
size_t TextContext::addCache(const std::vector<llama_token> &tokens) {
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
void TextContext::addCache(const std::vector<llama_token> &tokens, size_t cacheMissIndex) {
    // truncate cached context starting from `cacheMissIndex`
    // (if it's after the cached context size, skip truncation because whole cached prompt matched)
    if (cacheMissIndex < cache.size()) {
        llama_memory_seq_rm(llama_get_memory(context), seqId, cacheMissIndex, -1);
    }

    // copy only new tokens into the cache
    cache.resize(tokens.size());
    std::copy(tokens.begin() + cacheMissIndex, tokens.end(), cache.begin() + cacheMissIndex);
}

size_t TextContext::findCache(const std::vector<llama_token> &tokens) {
    size_t searchLength = std::min(tokens.size(), cache.size());

    // loop through the cache until `tokens[i]` does not match `cache[i]` anymore
    size_t cacheMissIndex = 0;
    while (cacheMissIndex < searchLength && tokens[cacheMissIndex] == cache[cacheMissIndex]) {
        cacheMissIndex++;
    }

    return cacheMissIndex;
}