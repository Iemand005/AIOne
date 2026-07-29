#include "OpenAIProvider.hpp"

using namespace AIOne;

OpenAIProvider::OpenAIProvider(std::string baseUrl, std::string apiKey) : http(baseUrl), apiKey(apiKey) {

}

std::vector<ModelInfo> OpenAIProvider::getModels() {

}