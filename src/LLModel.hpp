#pragma once

#include <string>
#include <iostream>
#include <functional>
#include <array>
#include <deque>
#include <mutex>
#include <thread>
#include <vector>

#include <llama-cpp.h>
#include <ggml-backend.h>
#include <llama-context.h>
#include <common/chat.h>

#include "Model.hpp"
#include "TextModelOptions.hpp"
#include "PreferredDevice.h"
#include "TextGenerationOptions.hpp"
#include "TextGenerationStats.hpp"
#include "MessageContext.hpp"
#include "MessageContextOptions.h"
#include "Message.hpp"
#include "LlamaUtil.hpp"

template<typename T>
using UniquePtr = std::unique_ptr<T>;

class LLModel : public Model
{
    // std::string modelPath;

    // struct Llama;
    // std::unique_ptr<Llama> llama;
    static void* llamaMalloc(size_t size) {
        return malloc(size);  // Or use your static runtime allocator
    }

    static void llamaFree(void* ptr) {
        free(ptr);
    }

    // llama.cpp stuff yup
    llama_model_ptr model;
    llama_context_ptr context = nullptr;
    llama_sampler_ptr sampler;

    std::thread generationWorker;

    std::vector<ggml_backend_dev_t> devices = {nullptr, nullptr};
    // std::shared_ptr<MessageContext> context = nullptr;
    const llama_vocab *vocab = nullptr;



    std::vector<std::shared_ptr<MessageContext>> registeredContexts;
    std::deque<llama_seq_id> freeSeqIds;
    llama_seq_id biggestSeqId = 0;
    bool generating = false;

    std::mutex mtx; // thread-safety

    void initBatch(llama_batch &batch, size_t maxBatchSize, llama_seq_id seqId) ;

    llama_model *getLlamaModel() ;

    /**
     * Warning: you are responsible for ensuring the `batchSize` does not exceed the batch's
     * `maxBatchSize`, else you will leak memory
     * 
     * Also, DO NOT EDIT the `tokens` vector AT ALL until you call `freeBatch` because
     * it just sets the tokens pointer to the vector data
     */
    void setBatch(llama_batch &batch, std::vector<llama_token> &tokens, size_t index, size_t batchSize) ;
    
    /**
     * Warning: you are responsible for ensuring the `batchSize` does not exceed the batch's
     * `maxBatchSize`, else you will leak memory
     */
    void setBatch(llama_batch &batch, llama_token *tokensStart, size_t batchSize) ;

    void freeBatch(llama_batch &batch) ;
    
public:
    typedef std::function<void(const float progress)> InputEvalCallback;
    typedef std::function<void(const std::string &token)> TokenCallback;

    const int maxTokens = 400;

    // LLModel(const std::string &path) : LLModel(path, TextModelOptions{}) {}

    LLModel(const std::string path, const TextModelOptions &options = TextModelOptions{});

    /*std::string getModelPath() {
        return modelPath;
    }*/

    bool isGenerating() {
        return generating;
    }



    // used by `TextContent`
    // checks if the llama context has been recreated
    bool isValid(llama_context *context) {
        return context == this->context.get();
    }

    // bool isGenerating() {
    //     return generating;
    // }

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

    MessageContext newContext() {
        return MessageContext(this, context.get());
    }

    std::shared_ptr<MessageContext> createContext() {
        auto context = std::make_shared<MessageContext>(newContext());
        registerContext(context);

        return context;
    }

    bool resetWithOptions(const MessageContextOptions &options) ;

    bool registerContext(std::shared_ptr<MessageContext> context) {
        if (!context->isConnectedTo(this)) {
            return false;
        }

        auto index = std::find(registeredContexts.begin(), registeredContexts.end(), context);

        if (index == registeredContexts.end()) {
            registeredContexts.push_back(context);
            return true;
        }

        return false;
    }

    bool unregisterContext(std::shared_ptr<MessageContext> context) {
        auto index = std::find(registeredContexts.begin(), registeredContexts.end(), context);

        if (index != registeredContexts.end()) {
            registeredContexts.erase(index);
            return true;
        }

        return false;
    }

    std::vector<llama_token> tokenize(std::string prompt, bool addSpecialTokens) ;

    void generateAsync(const std::string& prompt, TokenCallback onToken = nullptr, InputEvalCallback onInputEval = nullptr, const TextGenerationOptions& options = TextGenerationOptions()) {
        generationWorker = std::thread([this, prompt, onToken, onInputEval, options]() {
            completeAny(prompt, onToken, onInputEval, options);
            // generationWorkers.
        });
    }

    /**
     * Complete using any registered context.
     */
    TextGenerationStats completeAny(
        std::string prompt,
        TokenCallback onToken = nullptr,
        InputEvalCallback onInputEval = nullptr,
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

        for (size_t i = 0; i < registeredContexts.size(); i++) {
            size_t cacheMissIndex = registeredContexts[i]->findCache(promptTokens);

            if (cacheMissIndex > bestCached) {
                bestCached = cacheMissIndex;
                bestIndex = i;
            }
        }

        // complete with the best matching registered context
        return complete(registeredContexts[bestIndex], promptTokens, bestCached, onToken, onInputEval, options);
    }

    TextGenerationStats complete(
        std::shared_ptr<MessageContext> messageContext,
        std::string prompt,
        TokenCallback onToken = nullptr,
        InputEvalCallback onInputEval = nullptr,
        TextGenerationOptions options = TextGenerationOptions()
    ) {
        std::vector promptTokens = tokenize(prompt, true);
        return complete(messageContext, promptTokens, messageContext->findCache(promptTokens), onToken, onInputEval, options);
    }

    TextGenerationStats complete(
        std::shared_ptr<MessageContext> messageContext,
        std::vector<llama_token> &promptTokens,
        size_t cacheMissIndex,
        TokenCallback onToken = nullptr,
        InputEvalCallback onInputEval = nullptr,
        TextGenerationOptions options = TextGenerationOptions()
    );

    std::string chatToPrompt(std::vector<std::string> userAssMessages) {
        std::vector<common_chat_msg> commonMsgs(userAssMessages.size());
        
        for (size_t i = 0; i < userAssMessages.size(); i++) {
            commonMsgs[i] = {
                /* role    = */ i % 2 == 0 ? "user" : "assistant",
                /* content = */ userAssMessages[i]
            };
        }

        return applyJinjaTemplate(llama_model_chat_template(model.get(), nullptr), commonMsgs);
    }

    std::string chatToPrompt(std::vector<Message> messages) {
        std::vector<common_chat_msg> commonMsgs(messages.size());
        
        for (size_t i = 0; i < messages.size(); i++) {
            const Message& message = messages[i];

            commonMsgs[i] = {
                /* role    = */ message.role,
                /* content = */ message.content
            };
        }

        return applyJinjaTemplate(llama_model_chat_template(model.get(), nullptr), commonMsgs);
    }

    void destroy()
    {
        // if (model != nullptr) { these are smart pointers
        //     llama_model_free(model.get());
        //     model = nullptr;
        // }

        // if (context != nullptr) {
        //     llama_free(context.get());
        //     context = nullptr;
        // }
    }

    ~LLModel()
    {
        destroy();
        // Smart pointers handle cleanup automatically
    }
};
