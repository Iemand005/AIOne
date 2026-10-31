#include "ModelSession.hpp"

namespace AIOne {

bool ModelSession::loadModel(const std::string& path) {

    std::printf("Loading model: %s\n", path.c_str());
    std::fflush(stdout);

    try {
        unloadModel();

        factory.loadBackends();

        LLModelOptions options;
        options.device = PreferredDevice::DGPU;
        options.offloadLayers = -1;
        options.useMmap = false;

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

std::string ModelSession::generate(
    const std::string& prompt,
    unsigned int maxTokens
) {
    if (!model || !context)
        throw std::runtime_error("No model loaded; call loadModel() first");

    TextGenOptions options;
    options.maxTokens = maxTokens;

    auto result = model->complete(context, prompt, options);
    return result.output.content;
}

void ModelSession::unloadModel() {
    context.reset();
    model.reset();
}

bool ModelSession::isLoaded() const {
    return model && context;
}

}