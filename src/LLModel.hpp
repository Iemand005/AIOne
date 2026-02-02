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

#include "Role.h"
#include "Model.hpp"
#include "LLModelOptions.hpp"
#include "PreferredDevice.h"
#include "TextGenerationOptions.hpp"
#include "TextGenerationStats.hpp"
#include "TextContext.hpp"
#include "TextContextOptions.h"
#include "Message.hpp"
#include "Chat.hpp"
#include "LlamaUtil.hpp"

typedef std::function<void(const float progress)> ProgressCallback;
typedef std::function<void(const std::string &token)> TokenCallback;
typedef std::function<void(const TextGenerationStats &output)> FinishCallback;

class LLModel : public Model
{
    // std::string modelPath;

    // struct Llama;
    // std::unique_ptr<Llama> llama;
    
    // llama.cpp stuff yup
    llama_model_ptr model;
    llama_context_ptr context = nullptr;
    llama_sampler_ptr sampler;
    
    std::thread generationWorker;
    
    std::vector<ggml_backend_dev_t> devices = {nullptr, nullptr};
    // std::shared_ptr<TextContext> context = nullptr;
    const llama_vocab *vocab = nullptr;
    
    
    
    std::vector<std::shared_ptr<TextContext>> registeredContexts;
    std::deque<llama_seq_id> freeSeqIds;
    llama_seq_id biggestSeqId = 0;
    bool generating = false;
    
    std::vector<std::thread> activeThreads;
    std::mutex threadsMutex; // thread-safety


    static void* llamaMalloc(size_t size) {
        return malloc(size);  // Or use your static runtime allocator
    }

    static void llamaFree(void* ptr) {
        free(ptr);
    }

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
    

    const int maxTokens = 400;

    // LLModel(const std::string &path) : LLModel(path, LLModelOptions{}) {}

    LLModel(const std::string path, const LLModelOptions &options = {}, ProgressCallback onProgress = nullptr);

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
        std::lock_guard<std::mutex> lock(threadsMutex);

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
        std::lock_guard<std::mutex> lock(threadsMutex);

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
        return TextContext(this, context.get());
    }

    std::shared_ptr<TextContext> createContext() {
        auto context = std::make_shared<TextContext>(newContext());
        registerContext(context);

        return context;
    }

    bool resetWithOptions(const TextContextOptions &options) ;

    bool registerContext(std::shared_ptr<TextContext> context) {
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

    bool unregisterContext(std::shared_ptr<TextContext> context) {
        auto index = std::find(registeredContexts.begin(), registeredContexts.end(), context);

        if (index != registeredContexts.end()) {
            registeredContexts.erase(index);
            return true;
        }

        return false;
    }

    std::vector<llama_token> tokenize(std::string prompt, bool addSpecialTokens) ;

    void generateAsync(std::shared_ptr<Chat> chat, TokenCallback onToken = nullptr) {
        generateAsync(chat, nullptr, onToken, nullptr);
    }

    void generateAsync(std::shared_ptr<Chat> chat, FinishCallback onDone = nullptr, TokenCallback onToken = nullptr, ProgressCallback onInputEval = nullptr) {
        std::shared_ptr<Message> message = std::make_shared<Message>(Role::Assistant, "");
        generateAsync(chatToPrompt(chat.get()), chat->getOptions(), [chat, message, onDone](TextGenerationStats result) {
            message->content = result.output;
            chat->addMessage(*message);
            if (onDone) onDone(result);
        }, onToken, onInputEval);
    }

    void generateAsync(const std::string& prompt, const TextGenerationOptions& options = TextGenerationOptions(), FinishCallback onDone = nullptr, TokenCallback onToken = nullptr, ProgressCallback onInputEval = nullptr) {
        std::lock_guard<std::mutex> lock(threadsMutex);
        activeThreads.emplace_back([this, prompt, options, onDone, onToken, onInputEval]() {
            auto output = completeAny(prompt, options, onToken, onInputEval);
            if (onDone) onDone(output);
        });
    }

    /**
     * Complete using any registered context.
     */
    TextGenerationStats completeAny(
        std::string prompt,
        TextGenerationOptions options = TextGenerationOptions(),
        TokenCallback onToken = nullptr,
        ProgressCallback onInputEval = nullptr
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
        std::shared_ptr<TextContext> messageContext,
        std::string prompt,
        TokenCallback onToken = nullptr,
        ProgressCallback onInputEval = nullptr,
        TextGenerationOptions options = TextGenerationOptions()
    ) {
        std::vector promptTokens = tokenize(prompt, true);
        return complete(messageContext, promptTokens, messageContext->findCache(promptTokens), onToken, onInputEval, options);
    }

    TextGenerationStats complete(
        std::shared_ptr<TextContext> messageContext,
        std::vector<llama_token> &promptTokens,
        size_t cacheMissIndex,
        TokenCallback onToken = nullptr,
        ProgressCallback onInputEval = nullptr,
        TextGenerationOptions options = TextGenerationOptions()
    );

    std::string chatToPrompt(Chat *chat) {
        return chatToPrompt(chat->getMessages());
    }

    std::string chatToPrompt(yup
    ) {
        std::vector<common_chat_msg> commonMsgs(messages.size());
        
        size_t i = 0;
        for (auto &message : messages)
            commonMsgs[i++] = {message.role, message.content};

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
        for (auto& thread : activeThreads)
            if (thread.joinable()) thread.join();
        destroy();
        // Smart pointers handle cleanup automatically
    }
};
