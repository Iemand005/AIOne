#pragma once

#include <string>
#include <functional>
#include <deque>
#include <mutex>
#include <thread>
#include <vector>

// #include <llama-cpp.h>
// #include <ggml-backend.h>
// #include <llama-context.h>
// #include <common/chat.h>

#include "Model.hpp"
#include "LLModelOptions.hpp"
#include "TextGenOptions.hpp"
#include "TextGenResult.hpp"
#include "TextContext.hpp"
#include "TextContextOptions.h"
#include "Message.hpp"
#include "Chat.hpp"
typedef int32_t llama_seq_id;
    struct llama_context;

class LLModel : public Model
{
    struct Impl;
    std::unique_ptr<Impl> impl;

    
    
    // llama.cpp stuff yup
    // llama_model_ptr model;
    // llama_context_ptr context = nullptr;
    // llama_sampler_ptr sampler;
    // const llama_vocab *vocab = nullptr;
    // std::deque<llama_seq_id> freeSeqIds;
    // llama_seq_id biggestSeqId = 0;
    
    std::thread generationWorker;

    
    std::vector<std::shared_ptr<TextContext>> registeredContexts;

    bool generating = false;
    
    std::vector<std::thread> activeThreads;
    std::mutex threadsMutex;

    static void* llamaMalloc(size_t size) {
        return malloc(size);
    }

    static void llamaFree(void* ptr) {
        free(ptr);
    }

    
    
public:
// void *getSecretThingy() ;

    LLModel(const std::string path, const LLModelOptions &options = {});

    Model *super() { return this; }

    bool isGenerating() { return generating; }
    // used by `TextContent`
    // checks if the llama context has been recreated
    // struct llama_seq_id;
    bool isValid(llama_context *context);
llama_seq_id LLModel::claimSeqId();




    TextContext newContext() ;

    std::shared_ptr<TextContext> createContext() {
        auto context = std::make_shared<TextContext>(newContext());
        registerContext(context);

        return context;
    }

    bool resetWithOptions(const TextContextOptions &options) ;

    bool registerContext(std::shared_ptr<TextContext> context) {
        if (!context->isConnectedTo(this))
            return false;

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

    void generateAsync(std::shared_ptr<Chat> chat, const AsyncTextGenOptions& options = {}) {
        auto messages = chat->getMessages();
        generateAsync(chatToPrompt(messages), options);
    }

    // Continue writing a draft message
    void generateAsync(std::shared_ptr<Chat> chat, Message draft, const AsyncTextGenOptions& options = {}) {
        auto messages = chat->getMessages();
        generateAsync(chatToPrompt(messages, draft), options);
    }

    void generateAsync(const std::string& prompt, const AsyncTextGenOptions& options = {}) {
        std::lock_guard<std::mutex> lock(threadsMutex);
        activeThreads.emplace_back([this, prompt, options]() {
            auto result = completeAny(prompt, options);
            if (options.onDone) options.onDone(result);
        });
    }

    llama_token getToken(std::string text) {
        auto tokens = tokenize(text, false);
        return !tokens.empty() ? tokens[0] : -1;
    }

    /**
     * Complete using any registered context.
     */
    TextGenResult completeAny(std::string prompt, const TextGenOptions options = {}) {
        if (registeredContexts.empty())
            throw std::runtime_error("No registered contexts; cannot complete request");

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
        return complete(registeredContexts[bestIndex], promptTokens, bestCached, options);
    }

    TextGenResult complete(
        std::shared_ptr<TextContext> messageContext,
        std::string prompt,
        TextGenOptions options = {}
    ) {
        std::vector promptTokens = tokenize(prompt, true);
        return complete(messageContext, promptTokens, messageContext->findCache(promptTokens), options);
    }

    TextGenResult complete(
        std::shared_ptr<TextContext> messageContext,
        std::vector<llama_token> &promptTokens,
        size_t cacheMissIndex,
        TextGenOptions options = {}
    );

    std::string chatToPrompt(Chat *chat) {
        return chatToPrompt(chat->getMessages());
    }

    std::string chatToPrompt(Chat *chat, Message draft) {
        return chatToPrompt(chat->getMessages(), draft);
    }

    std::vector<common_chat_msg> toCommonMessages(std::vector<Message> messages) {
        std::vector<common_chat_msg> commonMsgs(messages.size());
        
        size_t i = 0;
        for (auto &message : messages) commonMsgs[i++] = {message.role, message.content};

        return commonMsgs;
    }

    std::string chatToPrompt(std::vector<Message> messages, bool addAss = true) ;

    std::string chatToPrompt(std::vector<Message> messages, Message &draft) ;

    void destroy()
    {
        
    }

    ~LLModel();
};

typedef std::unique_ptr<LLModel> LLModelPtr;
