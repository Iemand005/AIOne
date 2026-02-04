#pragma once

#include <string>
#include <functional>
#include <deque>
#include <mutex>
#include <thread>
#include <vector>

#include <llama-cpp.h>
#include <ggml-backend.h>
#include <llama-context.h>
#include <common/chat.h>

#include "Model.hpp"
#include "LLModelOptions.hpp"
#include "TextGenOptions.hpp"
#include "TextGenResult.hpp"
#include "TextContext.hpp"
#include "TextContextOptions.h"
#include "Message.hpp"
#include "Chat.hpp"
#include "LlamaUtil.hpp"


class LLModel : public Model
{
    // struct Llama;
    // std::unique_ptr<Llama> llama;
    
    // llama.cpp stuff yup
    llama_model_ptr model;
    llama_context_ptr context = nullptr;
    llama_sampler_ptr sampler;
    
    std::thread generationWorker;

    const llama_vocab *vocab = nullptr;
    
    std::vector<std::shared_ptr<TextContext>> registeredContexts;
    std::deque<llama_seq_id> freeSeqIds;
    llama_seq_id biggestSeqId = 0;
    bool generating = false;
    
    std::vector<std::thread> activeThreads;
    std::mutex threadsMutex;

    static void* llamaMalloc(size_t size) {
        return malloc(size);
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

    LLModel(const std::string path, const LLModelOptions &options = {});

    bool isGenerating() { return generating; }
    // used by `TextContent`
    // checks if the llama context has been recreated
    bool isValid(llama_context *context) { return context == this->context.get(); }

    llama_seq_id claimSeqId() {
        std::lock_guard<std::mutex> lock(threadsMutex);

        // get the first explicitly released seq id, or a new seq id
        if (freeSeqIds.empty()) {
            if (biggestSeqId == 0xFFFF) throw std::runtime_error("Maximum number of sequences reached (" + std::to_string(biggestSeqId) + ")");
            
            return biggestSeqId++; // no explicitly released seq ids, get a new one
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

    std::string chatToPrompt(std::vector<Message> messages, bool addAss = true) {
        auto commonMsgs = toCommonMessages(messages);
        return applyJinjaTemplate(model.get(), commonMsgs, addAss);
    }

    std::string chatToPrompt(std::vector<Message> messages, Message &draft) {
        messages.push_back(draft);
        auto prompt = chatToPrompt(messages, false);
        auto vocab = llama_model_get_vocab(this->model.get());

        std::string eosToken = common_token_to_piece(vocab, llama_vocab_eos(vocab), true);

        string_remove_suffix(prompt, "\n");
        string_remove_suffix(prompt, eosToken);
        return prompt;
    }

    void destroy()
    {
        
    }

    ~LLModel()
    {
        for (auto& thread : activeThreads)
            if (thread.joinable()) thread.join();
        destroy();
        // Smart pointers handle cleanup automatically
    }
};

typedef std::unique_ptr<LLModel> LLModelPtr;
