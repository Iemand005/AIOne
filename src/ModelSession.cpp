#include "ModelSession.hpp"

namespace AIOne {

bool ModelSession::loadModel(const std::string& path, const ProgressCallback& onProgress) {
    return loadModelWithOptions(path, {}, onProgress);
}

bool ModelSession::loadModelWithOptions(
    const std::string& path,
    const LoadOptions& loadOptions,
    const ProgressCallback& onProgress
) {

    std::printf("Loading model: %s\n", path.c_str());
    std::fflush(stdout);

    try {
        unloadModel();

        factory.loadBackends();

        LLModelOptions options;
        options.device = PreferredDevice::DGPU;
        options.offloadLayers = -1;
        options.useMmap = false;
        options.noHost = loadOptions.noHost;
        options.contextLength = loadOptions.contextLength;
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
    GenerateOptions options;
    options.maxTokens = maxTokens;
    return generateWithOptions(prompt, options, callbacks);
}

TextGenResult ModelSession::generateWithOptions(
    const std::string& prompt,
    const GenerateOptions& generateOptions,
    const GenerateCallbacks& callbacks
) {
    if (!model || !context)
        throw std::runtime_error("No model loaded; call loadModel() first");

    TextGenOptions options;
    options.maxTokens = generateOptions.maxTokens;
    options.minP = generateOptions.minP;
    options.topK = generateOptions.topK;
    options.temperature = generateOptions.temperature;
    options.seed = generateOptions.seed;
    options.onToken = callbacks.onToken;
    options.onTokenReasoning = callbacks.onTokenReasoning;
    options.onInputEval = callbacks.onInputEval;
    options.onGenerationStart = callbacks.onGenerationStart;
    options.onThinkStateChange = callbacks.onThinkStateChange;
    options.shouldStop = callbacks.shouldStop;

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