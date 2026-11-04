#include "ModelSession.hpp"

namespace AIOne {

bool ModelSession::loadModel(const std::string& path, const ProgressCallback& onProgress) {

    std::printf("Loading model: %s\n", path.c_str());
    std::fflush(stdout);

    try {
        unloadModel();

        factory.loadBackends();

        LLModelOptions options;
        options.device = PreferredDevice::DGPU;
        options.offloadLayers = -1;
        options.useMmap = false;
        options.onProgress = onProgress;

        model = factory.loadLLM(path, options);
        context = model->createContext();
    } catch (const std::exception& e) {
        std::printf("C++ exception: %s\n", e.what());
        std::fflush(stdout);
        throw;
    }

    std::printf("Model loaded\n");
    std::fflush(stdout);
    return true;
}

TextGenResult ModelSession::generate(
    const std::string& prompt,
    unsigned int maxTokens,
    const GenerateCallbacks& callbacks
) {
    if (!model || !context)
        throw std::runtime_error("No model loaded; call loadModel() first");

    TextGenOptions options;
    options.maxTokens = maxTokens;
    options.onToken = callbacks.onToken;
    options.onTokenReasoning = callbacks.onTokenReasoning;
    options.onInputEval = callbacks.onInputEval;
    options.onGenerationStart = callbacks.onGenerationStart;
    options.onThinkStateChange = callbacks.onThinkStateChange;

    return model->complete(context, prompt, options);
}

std::string ModelSession::formatChatPrompt(
    const std::vector<Message>& messages,
    bool addGenerationPrompt
) {
    if (!model)
        throw std::runtime_error("No model loaded; call loadModel() first");

    return model->chatToPrompt(messages, addGenerationPrompt);
}

std::string ModelSession::formatChatDraftPrompt(
    const std::vector<Message>& messages,
    Message draft
) {
    if (!model)
        throw std::runtime_error("No model loaded; call loadModel() first");

    return model->chatToPrompt(messages, draft);
}

void ModelSession::unloadModel() {
    context.reset();
    model.reset();
}

bool ModelSession::isLoaded() const {
    return model && context;
}

}