

#include <llama-cpp.h>
#include <ggml-backend.h>
#include <llama-context.h>
#include <common/chat.h>

// #include "LLModel.hpp"
#include "LLModelImpl.hpp"


std::string LLModel::chatToPrompt(std::vector<Message> messages, bool addAss = true) {
    auto commonMsgs = toCommonMessages(messages);
    return applyJinjaTemplate(impl->model.get(), commonMsgs, addAss);
}

std::string LLModel::chatToPrompt(std::vector<Message> messages, Message &draft) {
        messages.push_back(draft);
        auto prompt = chatToPrompt(messages, false);
        auto vocab = llama_model_get_vocab(impl->model.get());

        std::string eosToken = common_token_to_piece(vocab, llama_vocab_eos(vocab), true);

        string_remove_suffix(prompt, "\n");
        string_remove_suffix(prompt, eosToken);
        return prompt;
    }

    TextContext LLModel::newContext() {
        return TextContext(this, impl->context.get());
    }


LLModel::LLModel(const std::string path, const LLModelOptions &options)
{
    selectDevice(options.device);

    // init model
    llama_model_params modelParams = llama_model_default_params();
    modelParams.devices = devices.data();
    modelParams.n_gpu_layers = options.offloadLayers;
    if (options.onProgress) {
        setProgressCallback(options.onProgress);
        modelParams.progress_callback_user_data = this;
        modelParams.progress_callback = [](float progress, void *data){
            auto model = (LLModel *)data;
            auto onProgress = model->progressCallback();
            if (onProgress) onProgress(progress);
            return true;
        };
    }

    char *cpath = _strdup(path.c_str());

    llama_model_ptr model(llama_model_load_from_file(cpath, modelParams));
    if (!model)
    {
        throw std::runtime_error("Failed to load model");
    }

    impl->model = std::move(model);
    impl->vocab = llama_model_get_vocab(impl->model.get());

    // create llama context
    if (!resetWithOptions(options)) {
        throw std::runtime_error("Failed to create context");
    }
}


LLModel::~LLModel()
    {
        for (auto& thread : activeThreads)
            if (thread.joinable()) thread.join();
        destroy();
        // Smart pointers handle cleanup automatically
    }





bool LLModel::resetWithOptions(const TextContextOptions &options) {
    if (generating) return false;

    // recreate context with new options
    // the `TextContext` class automatically throws when their
    // llama context does not match the new llama context
    llama_context_params contextParams = llama_context_default_params();
    contextParams.n_ctx = options.contextLength;
    contextParams.n_batch = options.evalBatchSize;
    contextParams.n_threads = options.threadCount;

    llama_context *context = llama_init_from_model(impl->model.get(), contextParams);

    if (context == nullptr)
        return false;

    impl->context.reset(context);

    return true;
}

std::vector<llama_token> LLModel::tokenize(std::string prompt, bool addSpecialTokens) {
    // first get length to allocate
    const int promptTokenLen = -llama_tokenize(impl->vocab, prompt.c_str(), prompt.size(), nullptr, 0, addSpecialTokens, true);
    std::vector<llama_token> promptTokens(promptTokenLen);

    // then write tokens into `promptTokens`
    if (llama_tokenize(impl->vocab, prompt.c_str(), prompt.size(), promptTokens.data(), promptTokenLen, addSpecialTokens, true) < 0)
    {
        throw std::runtime_error("Failed to tokenize the prompt");
    }

    return promptTokens;
}



TextGenResult LLModel::complete(
    std::shared_ptr<TextContext> textContext,
    std::vector<llama_token> &promptTokens,
    size_t cacheMissIndex,
    TextGenOptions options
) {
    generating = true;

    bool thinking = false;
    llama_token thinkStartToken = getToken("<think>");
    llama_token thinkEndToken = getToken("</think>");

    // cache prompt and keep only the ones that were not already in the cache
    textContext->addCache(promptTokens, cacheMissIndex);
    
    if (cacheMissIndex > 0)
        promptTokens.erase(promptTokens.begin(), promptTokens.begin() + cacheMissIndex);

    std::cout << std::to_string(cacheMissIndex) + " cached tokens, " + std::to_string(promptTokens.size()) + " tokens to be evaluated" << std::endl;

    // init sampler with options
    llama_sampler_chain_params samplerParams = llama_sampler_chain_default_params();
    samplerParams.no_perf = false; // TODO disable?

    impl->sampler = llama_sampler_ptr(llama_sampler_chain_init(samplerParams));
    llama_sampler_chain_add(impl->sampler.get(), llama_sampler_init_min_p(options.minP, 1));
    llama_sampler_chain_add(impl->sampler.get(), llama_sampler_init_temp(options.temperature));
    llama_sampler_chain_add(impl->sampler.get(), llama_sampler_init_dist(options.seed));

    uint32_t maxBatchSize = textContext->getBatchSize();
    
    // create a batch once and reuse it
    llama_batch batch = {}; // init pointers with nullptr (see llama_batch_get_one), the `{}` is required
    impl->initBatch(batch, maxBatchSize, textContext->getSeqId());

    for (size_t i = 0; i < promptTokens.size(); i += maxBatchSize)
    {
        size_t remainingTokens = promptTokens.size() - i;

        // set tokens in batch (size of remaining prompt tokens, max `maxBatchSize` length)
        size_t batchSize = std::min(remainingTokens, (size_t)maxBatchSize);
        impl->setBatch(batch, promptTokens, i, batchSize);

        // if the model has an encoder, process input using encoder
        // in that case, the decoder is used on the "decoder start token" after this loop
        // this seems to be for T5?
        //
        // if the model does not have an encoder, the decoder will be used
        // in that case, the last batch will not be decoded, because that'll be done after this loop
        if (llama_model_has_encoder(impl->model.get()))
        {
            if (llama_encode(impl->context.get(), batch))
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

            if (llama_decode(impl->context.get(), batch))
            {
                generating = false;
                throw std::runtime_error("Failed to evaluate input tokens");
            }
        }

        if(options.onInputEval) options.onInputEval((float)i / promptTokens.size());
    }

    if (llama_model_has_encoder(impl->model.get()))
    {
        llama_token startTokenId = llama_model_decoder_start_token(impl->model.get());
        if (startTokenId == LLAMA_TOKEN_NULL)
            startTokenId = llama_vocab_bos(impl->vocab);

        // this batch will be processed in generation loop
        impl->setBatch(batch, &startTokenId, 1);
    }

    // context = messageContext->getContext()

    // generation loop
    size_t pos = 0;
    size_t maxPos = promptTokens.size() + options.maxTokens;
    size_t tokensGenerated = 0;
    std::string output;
    output.reserve(156); // why 156? I dunno, it's yummi :3

    while (pos + batch.n_tokens < maxPos)
    {
        // decode last batch (either input or the newly generated token)
        if (llama_decode(impl->context.get(), batch)) // TODO if it fails, it fucks up the context :<
        {
            generating = false;
            throw std::runtime_error("Failed to evaluate input tokens");
        }

        pos += batch.n_tokens;

        // generate a new token
        llama_token newTokenId = llama_sampler_sample(impl->sampler.get(), impl->context.get(), -1);

        // if "end of generation" token emitted, stop generation
        if (llama_vocab_is_eog(impl->vocab, newTokenId))
            break;

        if (newTokenId == thinkStartToken) {
            thinking = true;
            if (options.onThinkStart) options.onThinkStart();
        }

        if (newTokenId == thinkEndToken) {
            thinking = false;
            if (options.onThinkStart) options.onThinkStart();
        }

        // convert token to text
        char tokenBuffer[128];
        bool outputSpecial = false;
        int tokenSize = llama_token_to_piece(impl->vocab, newTokenId, tokenBuffer, sizeof(tokenBuffer), 0, outputSpecial);
        if (tokenSize < 0)
        {
            generating = false;
            throw std::runtime_error("Failed to evaluate input tokens");
        }

        // write text
        std::string text(tokenBuffer, tokenSize);

        output.append(text);
        if (options.onToken) options.onToken(text);
        if (options.onTokenReasoning) options.onTokenReasoning(text, thinking);

        // wrap token in batch for decoding
        impl->setBatch(batch, &newTokenId, 1);

        tokensGenerated++;
    }

    impl->freeBatch(batch);
    // llama_sampler_free(sampler.get()); important: do not free if the sampler has been added to a llama_sampler_chain (via llama_sampler_chain_add)

    generating = false;

    return {
        /*tokensEvaluated = */ promptTokens.size(),
        /*tokensGenerated = */ tokensGenerated,
        /*tokensCached    = */ cacheMissIndex,
        /*output          = */ Message{Role::Assistant, output}
    };
}
