

#include <llama-cpp.h>
#include <ggml-backend.h>
#include <llama-context.h>
#include <common/chat.h>

#include "LlamaUtil.hpp"


#pragma once

#include "LLModel.hpp"

struct LLModel::Impl {
    llama_model_ptr model;
    llama_context_ptr context = nullptr;
    llama_sampler_ptr sampler;

    const llama_vocab *vocab = nullptr;

    std::deque<llama_seq_id> freeSeqIds;
    llama_seq_id biggestSeqId = 0;

    // void initBatch(llama_batch &batch, size_t maxBatchSize, llama_seq_id seqId) ;

    // llama_model *getLlamaModel() ;

    /**
     * Warning: you are responsible for ensuring the `batchSize` does not exceed the batch's
     * `maxBatchSize`, else you will leak memory
     * 
     * Also, DO NOT EDIT the `tokens` vector AT ALL until you call `freeBatch` because
     * it just sets the tokens pointer to the vector data
     */
    // void setBatch(llama_batch &batch, std::vector<llama_token> &tokens, size_t index, size_t batchSize) ;
    
    // /**
    //  * Warning: you are responsible for ensuring the `batchSize` does not exceed the batch's
    //  * `maxBatchSize`, else you will leak memory
    //  */
    // void setBatch(llama_batch &batch, llama_token *tokensStart, size_t batchSize) ;

    // void freeBatch(llama_batch &batch) ;

    // llama_seq_id claimSeqId() {
    //     // get the first explicitly released seq id, or a new seq id
    //     if (freeSeqIds.empty()) {
    //         if (biggestSeqId == 0xFFFF) throw std::runtime_error("Maximum number of sequences reached (" + std::to_string(biggestSeqId) + ")");
            
    //         return biggestSeqId++; // no explicitly released seq ids, get a new one
    //     }

    //     // a seq id somewhere in the middle was released, use that one
    //     llama_seq_id seqId = freeSeqIds.front();
    //     freeSeqIds.pop_front();

    //     return seqId;
    // }

    // TODO this method can be optimized for memory, though not too important:
    // - claim seqId (0)
    // - claim seqId (1)
    // - release seqId 0 (added to freeSeqIds)
    // - release seqId 1 (decremented biggestSeqId)
    // result: all seqIds released, but `freeSeqIds` is not empty and `biggestSeqId` is 1
    // I don't really care about this much personally but feel free to fix it
    void releaseSeqId(llama_seq_id seqId) {

        // if the seq id to be released was the last one that was claimed,
        // just decrement the last claimed seq id
        if (biggestSeqId == seqId + 1) {
            biggestSeqId--;
            return;
        }

        // this seq id was somewhere in the middle, so add it to released ids
        freeSeqIds.push_back(seqId);
    }

    void initBatch(llama_batch &batch, size_t maxBatchSize, llama_seq_id seqId){
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

// bool 

// llama_model *LLModel::getLlamaModel() {
//     return model.get();
// }
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
    if (batch.seq_id && batch.seq_id[0])
        llamaFree(batch.seq_id[0]); // free the one array that's reused for all other seq_id items
    if (batch.seq_id)
        llamaFree(batch.seq_id); // free the array that was holding the pointers to that one array
    if (batch.n_seq_id)
        llamaFree(batch.n_seq_id);
    batch.token = nullptr; // stop referencing the vector
}

std::vector<common_chat_msg> toCommonMessages(std::vector<Message> messages) {
        std::vector<common_chat_msg> commonMsgs(messages.size());
        
        size_t i = 0;
        for (auto &message : messages) commonMsgs[i++] = {message.role, message.content};

        return commonMsgs;
    }
};

bool LLModel::isValid(llama_context *context)  { return context == impl->context.get(); }

// void *LLModel::getSecretThingy() {
//         return impl.get();
//     }

    llama_seq_id LLModel::claimSeqId() {
        // get the first explicitly released seq id, or a new seq id
        if (impl->freeSeqIds.empty()) {
            if (impl->biggestSeqId == 0xFFFF) throw std::runtime_error("Maximum number of sequences reached (" + std::to_string(impl->biggestSeqId) + ")");
            
            return impl->biggestSeqId++; // no explicitly released seq ids, get a new one
        }

        // a seq id somewhere in the middle was released, use that one
        llama_seq_id seqId = impl->freeSeqIds.front();
        impl->freeSeqIds.pop_front();

        return seqId;
    }

std::string LLModel::chatToPrompt(std::vector<Message> messages, bool addAss ) {
    auto commonMsgs = impl->toCommonMessages(messages);
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


LLModel::LLModel(const std::string path, const LLModelOptions &options) : impl(std::make_unique<Impl>())
{
    selectDevice(options.device);

    // init model
    llama_model_params modelParams = llama_model_default_params();
    modelParams.devices = devices.data();
    modelParams.n_gpu_layers = options.offloadLayers;
    modelParams.use_mmap = options.useMmap;
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
        throw std::runtime_error("Failed to load model");
    

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
        bool specialToken = false;

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
            specialToken = true;
            if (options.onThinkStart) options.onThinkStart();
        }

        if (newTokenId == thinkEndToken) {
            thinking = false;
            specialToken = true;
            if (options.onThinkStart) options.onThinkStart();
        }

        if (!specialToken) {
        // convert token to text
        char tokenBuffer[128];
        bool outputSpecial = false;
        int tokenSize = llama_token_to_piece(impl->vocab, newTokenId, tokenBuffer, sizeof(tokenBuffer), 0, outputSpecial);
        if (tokenSize < 0)
            throw std::runtime_error("Failed to evaluate input tokens");

        std::string text(tokenBuffer, tokenSize);

        output.append(text);
            if (options.onToken) options.onToken(text);
            if (options.onTokenReasoning) options.onTokenReasoning(text, thinking);
        }

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
