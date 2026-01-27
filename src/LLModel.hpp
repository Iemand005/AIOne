#pragma once

#include <string>
#include <iostream>
#include <functional>

#include <llama-cpp.h>

#include "model.h"

class LLModel : public Model
{
    llama_model_ptr model;
    std::array<ggml_backend_dev_t, 2> devices = {nullptr, nullptr};
    std::string modelPath;
    std::string lastPrompt;
public:
    typedef std::function<void(const std::string &token)> TokenCallback;

    const int maxTokens = 400;

    LLModel() {}

    LLModel(const std::string &path)
    {
        loadModel(path);
    }

    bool loadModel(const std::string &path)
    {
        modelPath = path;

        devices[0] = ggml_backend_dev_by_type(GGML_BACKEND_DEVICE_TYPE_GPU);
        if (!devices[0])
            devices[0] = ggml_backend_dev_by_type(GGML_BACKEND_DEVICE_TYPE_IGPU);
        if (!devices[0])
            devices[0] = ggml_backend_dev_by_type(GGML_BACKEND_DEVICE_TYPE_CPU);

        try
        {

            llama_model_params model_params = llama_model_default_params();
            model_params.devices = devices.data();
            llama_model_ptr model(llama_model_load_from_file(modelPath.c_str(), model_params));
            if (!model)
            {
                std::cerr << "Failed to load model" << std::endl;
            }

            this->model = std::move(model);
        }
        catch (std::exception ex)
        {
            std::cerr << "Modeal loading exceot" << ex.what();
        }

        return true;
    }
    // std::string prompt(std::string prompt) {
    //     std::string response;
    //     // prompt(prompt, [&response](std::string &token) {
    //     //     // *response =  *response + token;
    //     // });
    //     return response;
    // }

    void prompt(std::string prompt, TokenCallback callback)
    {
        lastPrompt = prompt;

        const llama_vocab *vocab = llama_model_get_vocab(model.get());
        const int promptTokenLen = -llama_tokenize(vocab, lastPrompt.c_str(), lastPrompt.size(), NULL, 0, true, true);

        // tokenize prompt
        std::vector<llama_token> promptTokens(promptTokenLen);
        if (llama_tokenize(vocab, lastPrompt.c_str(), lastPrompt.size(), promptTokens.data(), promptTokenLen, true, true) < 0)
        {
            std::cerr << "Failed to tokenize the prompt" << std::endl;
        }

        auto contextParams = llama_context_default_params();
        contextParams.n_ctx = promptTokenLen + maxTokens - 1; // context size
        contextParams.n_batch = 512;

        llama_context *context = llama_init_from_model(model.get(), contextParams);

        if (context == NULL)
        {
            std::cerr << "Failed to initialize a context window" << std::endl;
        }

        auto samplerParams = llama_sampler_chain_default_params();
        samplerParams.no_perf = false; // TODO disable?

        llama_sampler *sampler = llama_sampler_chain_init(samplerParams);
        llama_sampler_chain_add(sampler, llama_sampler_init_greedy());

        llama_batch batch;

        for (size_t i = 0; i < promptTokenLen; i += contextParams.n_batch)
        {
            size_t remainingTokens = promptTokenLen - i;

            // create a new batch (size of remaining prompt tokens, max `n_batch` length)
            size_t batchSize = std::min(remainingTokens, (size_t)contextParams.n_batch);
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

    void Destroy()
    {
        llama_model_free(model.get());
    }

    ~LLModel()
    {
        // Smart pointers handle cleanup automatically
    }
};
