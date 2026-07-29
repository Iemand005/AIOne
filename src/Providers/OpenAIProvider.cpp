#include "OpenAIProvider.hpp"

#include <nlohmann/json.hpp>
#include "ILLMProvider.hpp"

using namespace AIOne;

OpenAIProvider::OpenAIProvider(std::string baseUrl, std::string apiKey) : http(baseUrl), apiKey(apiKey) {

}

std::vector<ModelInfo> OpenAIProvider::getModels() {
	httplib::Headers headers = {
		{"Authorization", "Bearer " + apiKey}
	};
	auto res = http.Get("/v1/models", headers);
	if (res.status != 200) return {};

	auto json = nlohmann::json::parse(res.body);
	std::vector<ModelInfo> models;
	for (auto& item : json["data"]) {
		models.push_back({ item["id"], item["owned_by"] });
	}
	return models;
}

void OpenAIProvider::complete(const std::string& model, const std::vector<Message>& messages, const AsyncTextGenOptions& options) {

}