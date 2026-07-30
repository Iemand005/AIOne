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
	nlohmann::json msgsJson = nlohmann::json::array();

	if (!options.systemPrompt.empty()) {
		msgsJson.push_back({
			{"role", "system"},
			{"content", options.systemPrompt}
		});
	}

	for (const auto& msg : messages) {
		msgsJson.push_back({
			{"role", msg.role},
			{"content", msg.content}
		});
	}

	nlohmann::json reqBody;
	reqBody["model"] = model;
	reqBody["messages"] = msgsJson;
	reqBody["max_tokens"] = options.maxTokens;
	reqBody["temperature"] = options.temperature;
  // reqBody["min_p"] = options.minP;
	reqBody["stream"] = true;
	if (options.seed != 0xFFFFFFFF) reqBody["seed"] = options.seed;

	std::string requestStr = reqBody.dump();

	httplib::Headers headers = {
		{"Authorization", "Bearer " + apiKey},
		{"Content-Type", "application/json"}
	};

	std::string content;
	std::string reasoning;
	bool thinking = false;
	std::string sseBuf;

	if (options.onGenerationStart) options.onGenerationStart();

	http.PostStream("/v1/chat/completions", requestStr, "application/json", headers,
		[&](const char* data, size_t len) -> bool {
			sseBuf.append(data, len);

			size_t pos;
			while ((pos = sseBuf.find("\n\n")) != std::string::npos) {
				std::string event = sseBuf.substr(0, pos);
				sseBuf.erase(0, pos + 2);

				if (event.empty() || event[0] == ':') continue;

				if (event.size() < 6 || event.substr(0, 6) != "data: ") continue;
				std::string payload = event.substr(6);
				if (payload == "[DONE]") continue;

				try {
					auto json = nlohmann::json::parse(payload);
					auto& choices = json["choices"];
					if (choices.empty()) continue;

					auto& choice = choices[0];
					auto& delta = choice["delta"];
					auto finishReason = choice.value("finish_reason", nlohmann::json());
					if (!finishReason.is_null()) return true;

					if (delta.contains("reasoning_content") && !delta["reasoning_content"].is_null()) {
						std::string token = delta["reasoning_content"];
						if (!thinking) {
							thinking = true;
							if (options.onThinkStateChange) options.onThinkStateChange(true);
						}
						reasoning += token;
						if (options.onTokenReasoning) options.onTokenReasoning(token, true);
					}

					if (delta.contains("content") && !delta["content"].is_null()) {
						std::string token = delta["content"];
						if (thinking) {
							thinking = false;
							if (options.onThinkStateChange) options.onThinkStateChange(false);
						}
						content += token;
						if (options.onToken) options.onToken(token);
					}
				} catch (...) {
				}
			}
			return true;
		}
	);

	if (thinking && options.onThinkStateChange) options.onThinkStateChange(false);

	uint64_t parentId = messages.empty() ? 0 : messages.back().id;
	Message output("assistant", content, parentId);
	output.finished();

	TextGenResult result{};
	result.tokensEvaluated = static_cast<size_t>(msgsJson.size());
	result.tokensGenerated = 0;
	result.tokensCached = 0;
	result.output = output;

	if (options.onDone) options.onDone(result);
}
