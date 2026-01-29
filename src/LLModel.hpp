#pragma once

#include <string>
#include <iostream>
#include <functional>
#include <array>
#include <deque>
#include <mutex>

#include <llama-cpp.h>
#include <ggml-backend.h>
// #include <common/common.h>

#include "Model.hpp"
#include "TextModelOptions.hpp"
#include "PreferredDevice.hpp"
#include "TextGenerationOptions.hpp"
#include "TextContext.hpp"
#include "TextContextOptions.hpp"

class LLModel : public Model
{
    std::string modelPath;

    // llama.cpp stuff
    llama_model_ptr model;
    std::array<ggml_backend_dev_t, 2> devices = {nullptr, nullptr};
    llama_context *context;
    const llama_vocab *vocab;
    llama_sampler sampler;

    std::vector<TextContext> registeredContexts;
    std::deque<llama_seq_id> freeSeqIds;
    llama_seq_id biggestSeqId;
    bool generating;

    std::mutex mtx; // thread-safety

    void initBatch(llama_batch &batch, size_t maxBatchSize, llama_seq_id seqId) {
        // note: `llama_batch_get_one` doesn't allow changing the sequence id (it's always 0)
        // another note: `llama_batch_init` mallocs memory for tokens while the vector
        // already has that memory too, so I'm gonna be a bad kitty and just set the
        // struct manually to point to the vector's data for memory efficiency
        // (it's not much but let me have my 4 kB of memory savings okay)

        if (maxBatchSize <= 0) {
            throw std::runtime_error("Cannot create a batch with maximum size " + maxBatchSize);
        }
        
        batch.n_tokens = maxBatchSize;

        // create an array with 1 item (seqId)
        // this pointer will be used for all pointers in `seq_id` because they all use
        // the same `seqId` anyways, no need to malloc more than necessary
        int32_t* seqIdArrayLen = (int32_t*)malloc(sizeof(int32_t));
        *seqIdArrayLen = 1;
        batch.n_seq_id = seqIdArrayLen;

        // this array is used for all items
        llama_seq_id* seqIdArray = (llama_seq_id*)malloc(sizeof(llama_seq_id));
        seqIdArray[0] = seqId;
        
        batch.seq_id = (llama_seq_id**)malloc(sizeof(llama_seq_id*) * (maxBatchSize + 1));
        for (size_t i = 0; i < maxBatchSize; ++i) {
            batch.seq_id[i] = seqIdArray;
        }
        batch.seq_id[maxBatchSize] = nullptr; // `llama_batch_init` does this so, so shall I
    }

    /**
     * Warning: you are responsible for ensuring the `batchSize` does not exceed the batch's
     * `maxBatchSize`, else you will leak memory
     * 
     * Also, DO NOT EDIT the `tokens` vector AT ALL until you call `freeBatch` because
     * it just sets the tokens pointer to the vector data
     */
    void setBatch(llama_batch &batch, std::vector<llama_token> &tokens, size_t index, size_t batchSize) {
        setBatch(batch, tokens.data() + index, batchSize);
    }
    
    /**
     * Warning: you are responsible for ensuring the `batchSize` does not exceed the batch's
     * `maxBatchSize`, else you will leak memory
     */
    void setBatch(llama_batch &batch, llama_token *tokensStart, size_t batchSize) {
        if (batchSize != batch.n_tokens) {
            batch.seq_id[batch.n_tokens] = batch.seq_id[0]; // reset previous nullptr seq_id to the actual seq_id
            batch.n_tokens = batchSize;
            batch.seq_id[batchSize] = nullptr; // set new last seq_id to nullptr
        }
        
        batch.token = tokensStart;
    }

    void freeBatch(llama_batch &batch) {
        free(batch.n_seq_id);
        free(batch.seq_id[0]); // free the one array that's reused for all other seq_id items
        free(batch.seq_id); // free the array that was holding the pointers to that one array
        batch.token = nullptr; // stop referencing the vector
    }
    
public:
    typedef std::function<void(const float progress)> InputEvalCallback;
    typedef std::function<void(const std::string &token)> TokenCallback;

    const int maxTokens = 400;

    LLModel(const std::string &path) : LLModel(path, TextModelOptions{}) {}    

    LLModel(const std::string &path, const TextModelOptions &options)
    {
        modelPath = path;
        
        // choose devices based on preferred in options
        // (leaks into next case if preferred device isn't available)
        switch (options.device) {
            case PreferredDevice::ANY:
            case PreferredDevice::DGPU:
                devices[0] = ggml_backend_dev_by_type(GGML_BACKEND_DEVICE_TYPE_GPU);
                if (devices[0]) break;
            case PreferredDevice::IGPU:
                devices[0] = ggml_backend_dev_by_type(GGML_BACKEND_DEVICE_TYPE_IGPU);
                if (devices[0]) break;
            case PreferredDevice::ACCELERATOR:
                devices[0] = ggml_backend_dev_by_type(GGML_BACKEND_DEVICE_TYPE_ACCEL);
                if (devices[0]) break;
            case PreferredDevice::CPU:
                devices[0] = ggml_backend_dev_by_type(GGML_BACKEND_DEVICE_TYPE_CPU);
                if (devices[0]) break;
            default:
                throw std::runtime_error("No compatible device found");
        }

        // init model
        llama_model_params modelParams = llama_model_default_params();
        modelParams.devices = devices.data();
        modelParams.n_gpu_layers = options.offloadLayers;

        llama_model_ptr model(llama_model_load_from_file(modelPath.c_str(), modelParams));
        if (!model)
        {
            throw std::runtime_error("Failed to load model");
        }

        this->model = std::move(model);
        vocab = llama_model_get_vocab(this->model.get());

        // create llama context
        if (!resetWithOptions(options)) {
            throw std::runtime_error("Failed to create context");
        }
    }

    std::string getModelPath() {
        return modelPath;
    }

    bool isGenerating() {
        return generating;
    }

    bool resetWithOptions(const TextContextOptions &options) {
        if (generating) {
            return false;
        }

        if (context != nullptr) {
            llama_free(context);
        }

        // recreate context with new options
        // the `TextContext` class automatically throws when their
        // llama context does not match the new llama context
        llama_context_params contextParams = llama_context_default_params();
        contextParams.n_ctx = options.contextLength;
        contextParams.n_batch = options.evalBatchSize;
        
        context = llama_init_from_model(model.get(), contextParams);
        if (context == nullptr) {
            return false;
        }

        return true;
    }

    // used by `TextContent`
    // checks if the llama context has been recreated
    bool isValid(llama_context *context) {
        return context == this->context;
    }

    llama_seq_id claimSeqId() {
        std::lock_guard<std::mutex> lock(mtx);

        // get the first explicitly released seq id, or a new seq id
        if (freeSeqIds.empty()) {
            if (biggestSeqId == 0xFFFF) {
                throw std::runtime_error("Maximum number of sequences reached (" + std::to_string(biggestSeqId) + ")");
            }
            
            // no explicitly released seq ids, get a new one
            return biggestSeqId++;
        }

        // a seq id somewhere in the middle was released, use that one
        llama_seq_id seqId = freeSeqIds.front();
        freeSeqIds.pop_front();

        return seqId;
    }

    // TODO this method can be optimized for memory, though not too important:
    // - claim seqId (0)
    // - claim seqId (1)
    // - release seqId 0 (added to freeSeqIds)
    // - release seqId 1 (decremented biggestSeqId)
    // result: all seqIds released, but `freeSeqIds` is not empty and `biggestSeqId` is 1
    // I don't really care about this much personally but feel free to fix it
    void releaseSeqId(llama_seq_id seqId) {
        std::lock_guard<std::mutex> lock(mtx);

        // if the seq id to be released was the last one that was claimed,
        // just decrement the last claimed seq id
        if (biggestSeqId == seqId + 1) {
            biggestSeqId--;
            return;
        }

        // this seq id was somewhere in the middle, so add it to released ids
        freeSeqIds.push_back(seqId);
    }

    TextContext newContext() {
        return TextContext(this, model.get(), context);
    }

    bool registerContext(TextContext context) {
        if (!context.isConnectedTo(this)) {
            return false;
        }

        auto index = std::find(registeredContexts.begin(), registeredContexts.end(), context);

        if (index == registeredContexts.end()) {
            registeredContexts.push_back(context);
            return true;
        }

        return false;
    }

    bool unregisterContext(TextContext context) {
        auto index = std::find(registeredContexts.begin(), registeredContexts.end(), context);

        if (index != registeredContexts.end()) {
            registeredContexts.erase(index);
            return true;
        }

        return false;
    }

    std::vector<llama_token> tokenize(std::string prompt, bool addSpecialTokens) {
        // first get length to allocate
        const int promptTokenLen = -llama_tokenize(vocab, prompt.c_str(), prompt.size(), nullptr, 0, addSpecialTokens, true);
        std::vector<llama_token> promptTokens(promptTokenLen);

        // then write tokens into `promptTokens`
        if (llama_tokenize(vocab, prompt.c_str(), prompt.size(), promptTokens.data(), promptTokenLen, addSpecialTokens, true) < 0)
        {
            throw std::runtime_error("Failed to tokenize the prompt");
        }

        return promptTokens;
    }

    /**
     * Complete using any registered context.
     */
    void completeAny(
        std::string prompt,
        TokenCallback onToken,
        InputEvalCallback onInputEval,
        TextGenerationOptions options = TextGenerationOptions()
    ) {
        if (registeredContexts.empty()) {
            throw std::runtime_error("No registered contexts; cannot complete request");
        }

        // tokenize prompt
        std::vector<llama_token> promptTokens = tokenize(prompt, true);
        
        // find the registered context with the most token cache hits
        size_t bestCached = 0;
        size_t bestIndex = 0;

        if (registeredContexts.size() > 1) {
            for (size_t i = 0; i < registeredContexts.size(); i++) {
                size_t cacheMissIndex = registeredContexts[i].findCache(promptTokens);

                if (cacheMissIndex > bestCached) {
                    bestCached = cacheMissIndex;
                    bestIndex = i;
                }
            }
        }

        // complete with the best matching registered context
        complete(registeredContexts[bestIndex], promptTokens, bestCached, onToken, onInputEval, options);
    }

    void complete(
        TextContext textContext,
        std::string prompt,
        TokenCallback onToken,
        InputEvalCallback onInputEval,
        TextGenerationOptions options = TextGenerationOptions()
    ) {
        std::vector promptTokens = tokenize(prompt, true);
        complete(textContext, promptTokens, textContext.findCache(promptTokens), onToken, onInputEval, options);
    }

    void complete(
        TextContext textContext,
        std::vector<llama_token> &promptTokens,
        size_t cacheMissIndex,
        TokenCallback onToken,
        InputEvalCallback onInputEval,
        TextGenerationOptions options = TextGenerationOptions()
    ) {
        generating = true;

        // cache prompt and keep only the ones that were not already in the cache
        textContext.addCache(promptTokens, cacheMissIndex);
        
        if (cacheMissIndex > 0) {
            promptTokens.erase(promptTokens.begin(), promptTokens.begin() + cacheMissIndex);
        }

        // init sampler with options
        llama_sampler_chain_params samplerParams = llama_sampler_chain_default_params();
        samplerParams.no_perf = false; // TODO disable?

        llama_sampler *sampler = llama_sampler_chain_init(samplerParams);
        llama_sampler_chain_add(sampler, llama_sampler_init_min_p(options.minP, 1));
        llama_sampler_chain_add(sampler, llama_sampler_init_temp(options.temperature));
        llama_sampler_chain_add(sampler, llama_sampler_init_dist(options.seed));

        uint32_t maxBatchSize = textContext.getBatchSize();
        
        // create a batch once and reuse it
        llama_batch batch = {}; // init pointers with nullptr (see llama_batch_get_one), the `{}` is required
        initBatch(batch, maxBatchSize, textContext.getSeqId());

        for (size_t i = 0; i < promptTokens.size(); i += maxBatchSize)
        {
            size_t remainingTokens = promptTokens.size() - i;

            // set tokens in batch (size of remaining prompt tokens, max `maxBatchSize` length)
            size_t batchSize = std::min(remainingTokens, (size_t)maxBatchSize);
            setBatch(batch, promptTokens, i, batchSize);

            // if the model has an encoder, process input using encoder
            // in that case, the decoder is used on the "decoder start token" after this loop
            // this seems to be for T5?
            //
            // if the model does not have an encoder, the decoder will be used
            // in that case, the last batch will not be decoded, because that'll be done after this loop
            if (llama_model_has_encoder(model.get()))
            {
                if (llama_encode(context, batch))
                {
                    generating = false;
                    throw std::runtime_error("Failed to evaluate input tokens");
                }
            }
            else
            {
                if (remainingTokens <= maxBatchSize)
                {
                    // this is the last batch, which will be processed in the generation loop
                    break;
                }

                if (llama_decode(context, batch))
                {
                    generating = false;
                    throw std::runtime_error("Failed to evaluate input tokens");
                }
            }

            onInputEval((float)i / promptTokens.size());
        }

        if (llama_model_has_encoder(model.get()))
        {
            llama_token startTokenId = llama_model_decoder_start_token(model.get());
            if (startTokenId == LLAMA_TOKEN_NULL)
                startTokenId = llama_vocab_bos(vocab);

            // this batch will be processed in generation loop
            setBatch(batch, &startTokenId, 1);
        }

        // generation loop
        size_t pos = 0;
        size_t maxPos = promptTokens.size() + maxTokens;
        while (pos + batch.n_tokens < maxPos)
        {
            // decode last batch (either input or the newly generated token)
            if (llama_decode(context, batch))
            {
                generating = false;
                throw std::runtime_error("Failed to evaluate input tokens");
            }

            pos += batch.n_tokens;

            // generate a new token
            llama_token newTokenId = llama_sampler_sample(sampler, context, -1);

            // if "end of generation" token emitted, stop generation
            if (llama_vocab_is_eog(vocab, newTokenId))
                break;

            // convert token to text
            char buf[128];
            int n = llama_token_to_piece(vocab, newTokenId, buf, sizeof(buf), 0, true);
            if (n < 0)
            {
                generating = false;
                throw std::runtime_error("Failed to evaluate input tokens");
            }

            // write text
            std::string text(buf, n);
            // std::cout << text.c_str();
            onToken(text);

            // wrap token in batch for decoding
            setBatch(batch, &newTokenId, 1);

            // TODO provide generation stats?
            // tokensGenerated++;
        }

        freeBatch(batch);
        llama_sampler_free(sampler);
        generating = false;
    }

    void destroy()
    {
        if (model != nullptr) {
            llama_model_free(model.get());
            model = nullptr;
        }

        if (context != nullptr) {
            llama_free(context);
            context = nullptr;
        }
    }

    ~LLModel()
    {
        destroy();
        // Smart pointers handle cleanup automatically
    }
};
