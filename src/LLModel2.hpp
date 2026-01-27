#pragma once

#include <string>
#include <iostream>
#include <functional>
#include <array>

#include <llama-cpp.h>
#include <ggml-backend.h>

#include "Model.hpp"
#include "ModelOptions.hpp"
#include "PreferredDevice.hpp"
#include "TextGenerationOptions.hpp"
#include "TextContext.hpp"
#include "TextContextOptions.hpp"

class LLModel : public Model
{
    llama_model_ptr model;
    std::array<ggml_backend_dev_t, 2> devices = {nullptr, nullptr};
    std::string modelPath;
    const llama_vocab *vocab;
    llama_sampler sampler;
    
public:
    typedef std::function<void(const std::string &token)> TokenCallback;

    const int maxTokens = 400;

    LLModel(const std::string &path, const ModelOptions &options)
    {
        modelPath = path;
        
        switch (options.device) {
            case PreferredDevice::ANY:
            case PreferredDevice::DGPU:
                devices[0] = ggml_backend_dev_by_type(GGML_BACKEND_DEVICE_TYPE_GPU);
                if (devices[0]) break;
            case PreferredDevice::IGPU:
                devices[0] = ggml_backend_dev_by_type(GGML_BACKEND_DEVICE_TYPE_IGPU);
                if (devices[0]) break;
            case PreferredDevice::CPU:
                devices[0] = ggml_backend_dev_by_type(GGML_BACKEND_DEVICE_TYPE_CPU);
                if (devices[0]) break;
            default:
                throw std::runtime_error("No compatible device found");
        }

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
    }

    std::string getModelPath() {
        return modelPath;
    }

    std::vector<llama_token> tokenize(std::string prompt, bool addSpecialTokens) {
        const int promptTokenLen = -llama_tokenize(vocab, prompt.c_str(), prompt.size(), NULL, 0, addSpecialTokens, true);
        std::vector<llama_token> promptTokens(promptTokenLen);

        if (llama_tokenize(vocab, prompt.c_str(), prompt.size(), promptTokens.data(), promptTokenLen, addSpecialTokens, true) < 0)
        {
            throw std::runtime_error("Failed to tokenize the prompt");
        }

        return promptTokens;
    }

    TextContext newContext() {
        return newContext(TextContextOptions());
    }

    TextContext newContext(TextContextOptions options) {
        return TextContext(model.get(), options);
    }

    void complete(TextContext context, std::string prompt, TokenCallback callback) {
        complete(context, prompt, callback, TextGenerationOptions());
    }

    void complete(TextContext context, std::string prompt, TokenCallback callback, TextGenerationOptions options) {
        // tokenize prompt
        std::vector<llama_token> promptTokens = tokenize(prompt, true);

        llama_sampler_chain_params samplerParams = llama_sampler_chain_default_params();
        samplerParams.no_perf = false; // TODO disable?

        llama_sampler *sampler = llama_sampler_chain_init(samplerParams);
        llama_sampler_chain_add(sampler, llama_sampler_init_min_p(options.minP, 1));
        llama_sampler_chain_add(sampler, llama_sampler_init_temp(options.temperature));
        llama_sampler_chain_add(sampler, llama_sampler_init_dist(options.seed));

        llama_batch batch;
        uint32_t maxBatchSize = context.getBatchSize();

        // TODO everything down here
        // I want to move the encoding/decoding somewhat to the context, since
        // that'll have to manage the cache and stuff, but I'm sure there's a better way
        // I'll look into it later :3
        // (perhaps a built-in way to compare against cached tokens)

        for (size_t i = 0; i < promptTokens.size(); i += maxBatchSize)
        {
            size_t remainingTokens = promptTokens.size() - i;

            // create a new batch (size of remaining prompt tokens, max `maxBatchSize` length)
            size_t batchSize = std::min(remainingTokens, (size_t)maxBatchSize);
            batch = llama_batch_get_one(promptTokens.data() + i, batchSize);

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
                    std::cerr << "Failed to evaluate input tokens" << std::endl;
                }
            }
            else
            {
                if (remainingTokens <= contextParams.n_batch)
                {
                    // this is the last batch, which will be processed in the generation loop
                    break;
                }

                if (llama_decode(context, batch))
                {
                    std::cerr << "Failed to evaluate input tokens" << std::endl;
                }
            }
        }

        if (llama_model_has_encoder(model.get()))
        {
            llama_token startTokenId = llama_model_decoder_start_token(model.get());
            if (startTokenId == LLAMA_TOKEN_NULL)
                startTokenId = llama_vocab_bos(vocab);

            // this batch will be processed in generation loop
            batch = llama_batch_get_one(&startTokenId, 1);
        }

        // generation loop
        size_t pos = 0;
        size_t maxPos = promptTokenLen + maxTokens;
        while (pos + batch.n_tokens < maxPos)
        {
            // decode last batch (either input or the newly generated token)
            if (llama_decode(context, batch))
            {
                std::cerr << "Failed to evaluate input tokens" << std::endl;
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
                std::cerr << "Failed to convert token to text" << std::endl;
            }

            // write text
            std::string text(buf, n);
            std::cout << text.c_str();
            callback(text);

            // wrap token in batch for decoding
            batch = llama_batch_get_one(&newTokenId, 1);

            // TODO provide generation stats?
            // tokensGenerated++;
        }

        llama_sampler_free(sampler);
        llama_free(context);
    }

    void destroy()
    {
        if (model != NULL) {
            llama_model_free(model.get());
            model = NULL;
        }
    }

    ~LLModel()
    {
        destroy();
        // Smart pointers handle cleanup automatically
    }
};
