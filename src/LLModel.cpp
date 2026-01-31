

#include "LLModel.hpp"

// struct LLModel::Llama {

// }

void LLModel::initBatch(llama_batch &batch, size_t maxBatchSize, llama_seq_id seqId){
        // note: `llama_batch_get_one` doesn't allow changing the sequence id (it's always 0)
        // another note: `llama_batch_init` mallocs memory for tokens while the vector
        // already has that memory too, so I'm gonna be a bad kitty and just set the
        // struct manually to point to the vector's data for memory efficiency
        // (it's not much but let me have my 4 kB of memory savings okay)

        if (maxBatchSize <= 0) {
            throw std::runtime_error("Cannot create a batch with maximum size " + maxBatchSize);
        }
        
        batch.n_tokens = maxBatchSize;

        // array with size `n_seq_id(maxBatchSize)`
        // each item signals the amount of `seq_id`s a token belongs to
        // in this case, always 1 (set in the loop a little further down the function)
        batch.n_seq_id = (int32_t*)llamaMalloc(sizeof(int32_t) * maxBatchSize);

        // this array is used for all items
        llama_seq_id* seqIdArray = (llama_seq_id*)llamaMalloc(sizeof(llama_seq_id));
        seqIdArray[0] = seqId;
        
        batch.seq_id = (llama_seq_id**)llamaMalloc(sizeof(llama_seq_id*) * (maxBatchSize + 1));
        for (size_t i = 0; i < maxBatchSize; ++i) {
            batch.n_seq_id[i] = 1;
            batch.seq_id[i] = seqIdArray;
        }
        batch.seq_id[maxBatchSize] = nullptr; // `llama_batch_init` does this so, so shall I
    }
llama_model *LLModel::getLlamaModel() {
        return model.get();
    }
    /**
     * Warning: you are responsible for ensuring the `batchSize` does not exceed the batch's
     * `maxBatchSize`, else you will leak memory
     * 
     * Also, DO NOT EDIT the `tokens` vector AT ALL until you call `freeBatch` because
     * it just sets the tokens pointer to the vector data
     */
    void LLModel::setBatch(llama_batch &batch, std::vector<llama_token> &tokens, size_t index, size_t batchSize) {
        setBatch(batch, tokens.data() + index, batchSize);
    }
    
    /**
     * Warning: you are responsible for ensuring the `batchSize` does not exceed the batch's
     * `maxBatchSize`, else you will leak memory
     */
    void LLModel::setBatch(llama_batch &batch, llama_token *tokensStart, size_t batchSize) {
        if (batchSize != batch.n_tokens) {
            batch.seq_id[batch.n_tokens] = batch.seq_id[0]; // reset previous nullptr seq_id to the actual seq_id
            batch.n_tokens = batchSize;
            batch.seq_id[batchSize] = nullptr; // set new last seq_id to nullptr
        }
        
        batch.token = tokensStart;
    }

    void LLModel::freeBatch(llama_batch &batch) {
        // free(batch.n_seq_id);
        // free(batch.seq_id[0]); // free the one array that's reused for all other seq_id items
        // free(batch.seq_id); // free the array that was holding the pointers to that one array
        if (batch.seq_id && batch.seq_id[0])
            llamaFree(batch.seq_id[0]);
        if (batch.seq_id)
            llamaFree(batch.seq_id);
        if (batch.n_seq_id)
            llamaFree(batch.n_seq_id);
        batch.token = nullptr; // stop referencing the vector
    }

    
    LLModel::LLModel(const std::string path, const TextModelOptions &options)
    {
        // modelPath = path;
        
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

        char *cpath = _strdup(path.c_str());

        llama_model_ptr model(llama_model_load_from_file(cpath, modelParams));
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

    bool LLModel::resetWithOptions(const MessageContextOptions &options) {
        if (generating) {
            return false;
        }

        // recreate context with new options
        // the `MessageContext` class automatically throws when their
        // llama context does not match the new llama context
        llama_context_params contextParams = llama_context_default_params();
        contextParams.n_ctx = options.contextLength;
        contextParams.n_batch = options.evalBatchSize;
        contextParams.n_threads = options.threadCount;

        llama_context *context = llama_init_from_model(model.get(), contextParams);

        if (context == nullptr) {
            return false;
        }

        this->context.reset(context);

        return true;
    }

    std::vector<llama_token> LLModel::tokenize(std::string prompt, bool addSpecialTokens) {
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

    TextGenerationStats LLModel::complete(
        std::shared_ptr<MessageContext> messageContext,
        std::vector<llama_token> &promptTokens,
        size_t cacheMissIndex,
        TokenCallback onToken,
        InputEvalCallback onInputEval,
        TextGenerationOptions options
    ) {
        generating = true;

        // cache prompt and keep only the ones that were not already in the cache
        messageContext->addCache(promptTokens, cacheMissIndex);
        
        if (cacheMissIndex > 0) {
            promptTokens.erase(promptTokens.begin(), promptTokens.begin() + cacheMissIndex);
        }

        std::cout << std::to_string(cacheMissIndex) + " cached tokens, " + std::to_string(promptTokens.size()) + " tokens to be evaluated" << std::endl;

        // init sampler with options
        llama_sampler_chain_params samplerParams = llama_sampler_chain_default_params();
        samplerParams.no_perf = false; // TODO disable?

        sampler = llama_sampler_ptr(llama_sampler_chain_init(samplerParams));
        llama_sampler_chain_add(sampler.get(), llama_sampler_init_min_p(options.minP, 1));
        llama_sampler_chain_add(sampler.get(), llama_sampler_init_temp(options.temperature));
        llama_sampler_chain_add(sampler.get(), llama_sampler_init_dist(options.seed));

        uint32_t maxBatchSize = messageContext->getBatchSize();
        
        // create a batch once and reuse it
        llama_batch batch = {}; // init pointers with nullptr (see llama_batch_get_one), the `{}` is required
        initBatch(batch, maxBatchSize, messageContext->getSeqId());

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
                if (llama_encode(context.get(), batch))
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

                if (llama_decode(context.get(), batch))
                {
                    generating = false;
                    throw std::runtime_error("Failed to evaluate input tokens");
                }
            }

            if(onInputEval)onInputEval((float)i / promptTokens.size());
        }

        if (llama_model_has_encoder(model.get()))
        {
            llama_token startTokenId = llama_model_decoder_start_token(model.get());
            if (startTokenId == LLAMA_TOKEN_NULL)
                startTokenId = llama_vocab_bos(vocab);

            // this batch will be processed in generation loop
            setBatch(batch, &startTokenId, 1);
        }

        // context = messageContext->getContext()

        // generation loop
        size_t pos = 0;
        size_t maxPos = promptTokens.size() + maxTokens;
        size_t tokensGenerated = 0;
        std::string output;
        output.reserve(156); // why 156? I dunno, it's yummi :3

        while (pos + batch.n_tokens < maxPos)
        {
            // decode last batch (either input or the newly generated token)
            if (llama_decode(context.get(), batch)) // TODO if it fails, it fucks up the context :<
            {
                generating = false;
                throw std::runtime_error("Failed to evaluate input tokens");
            }

            pos += batch.n_tokens;

            // generate a new token
            llama_token newTokenId = llama_sampler_sample(sampler.get(), context.get(), -1);

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
            output.append(text);
            if (onToken) onToken(text);

            // wrap token in batch for decoding
            setBatch(batch, &newTokenId, 1);

            tokensGenerated++;
        }

        freeBatch(batch);
        // llama_sampler_free(sampler.get()); important: do not free if the sampler has been added to a llama_sampler_chain (via llama_sampler_chain_add)

        generating = false;

        return {
            /*tokensEvaluated = */ promptTokens.size(),
            /*tokensGenerated = */ tokensGenerated,
            /*tokensCached    = */ cacheMissIndex,
            /*output          = */ output
        };
    }
