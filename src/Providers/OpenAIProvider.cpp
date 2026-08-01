#include "OpenAIProvider.hpp"

#include <nlohmann/json.hpp>
#include "ILLMProvider.hpp"

using namespace AIOne;

namespace {
// Make sure the base URL ends with exactly one "/v1" segment so the request
// paths below ("/models", "/chat/completions") resolve to the right endpoints.
// Handles URLs that already end with "/v1", accidentally repeat it ("/v1/v1"),
// or have no "/v1" at all.
std::string normalizeBaseUrl(std::string url) {
	while (!url.empty() && url.back() == '/') url.pop_back();

	while (url.size() >= 3 &&
	       (url.compare(url.size() - 3, 3, "/v1") == 0 ||
	        url.compare(url.size() - 3, 3, "/V1") == 0)) {
		url.resize(url.size() - 3);
		while (!url.empty() && url.back() == '/') url.pop_back();
	}

	url += "/v1";
	return url;
}

// Pull a human-readable message out of an error response body. Handles the
// common shapes: OpenAI {"error":{"message":...}}, {"error":"..."},
// FastAPI {"detail":"..."} / {"detail":[{...,"msg":"...",...}]},
// and plain {"message":...}. Falls back to the raw body otherwise.
std::string extractApiErrorMessage(const std::string& body, int status) {
	auto cap = [](const std::string& s) {
		return s.size() <= 4000 ? s : s.substr(0, 4000) + "...";
	};

	if (body.empty()) return "HTTP " + std::to_string(status);

	nlohmann::json j;
	try {
		j = nlohmann::json::parse(body);
	} catch (...) {
		return "HTTP " + std::to_string(status) + ": " + cap(body);
	}

	std::string msg;
	if (j.is_object()) {
		if (j.contains("error")) {
			auto& e = j["error"];
			if (e.is_object() && e.contains("message") && e["message"].is_string())
				msg = e["message"].get<std::string>();
			else if (e.is_string())
				msg = e.get<std::string>();
		}
		if (msg.empty() && j.contains("detail")) {
			auto& d = j["detail"];
			if (d.is_string())
				msg = d.get<std::string>();
			else if (d.is_array()) {
				for (auto& item : d) {
					if (!item.is_object()) continue;
					std::string loc = item.value("loc", nlohmann::json()).dump();
					std::string m = item.value("msg", "");
					if (!msg.empty()) msg += "\n";
					if (!loc.empty() && loc != "null") msg += loc + ": ";
					msg += m;
				}
			}
		}
		if (msg.empty() && j.contains("message") && j["message"].is_string())
			msg = j["message"].get<std::string>();
	}

	if (msg.empty())
		msg = "HTTP " + std::to_string(status) + ": " + cap(body);
	else
		msg = "HTTP " + std::to_string(status) + ": " + msg;
	return msg;
}
}}

OpenAIProvider::OpenAIProvider(std::string baseUrl, std::string apiKey) : http(normalizeBaseUrl(baseUrl)), apiKey(apiKey) {

}

	std::vector<ModelInfo> OpenAIProvider::getModels() {
	httplib::Headers headers = {
		{"Authorization", "Bearer " + apiKey}
	};
	lastError.clear();
	auto res = http.Get("/models", headers);
	if (res.status != 200) {
		lastError = "HTTP " + std::to_string(res.status);
		if (!res.body.empty() && res.body.size() < 500)
			lastError += ": " + res.body;
		return {};
	}

	std::vector<ModelInfo> models;
	try {
		auto json = nlohmann::json::parse(res.body);
		if (!json.contains("data") || !json["data"].is_array()) {
			lastError = "Unexpected response (no \"data\" array): "
				+ (res.body.size() < 500 ? res.body : res.body.substr(0, 500));
			return models;
		}

		for (auto& item : json["data"]) {
			// Some providers omit "owned_by" (e.g. OpenRouter has "name" instead);
			// never access fields with operator[] to avoid throwing on null.
			std::string id = item.value("id", "");
			if (id.empty()) continue;
			std::string provider = item.value("owned_by", item.value("name", ""));
			models.push_back({ id, provider });
		}
	} catch (const std::exception& e) {
		lastError = std::string("Failed to parse response: ") + e.what();
		return {};
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
	if (options.maxTokens > 0) reqBody["max_tokens"] = options.maxTokens;
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
	std::string errBuf;
	std::string errMsg;

	auto fail = [&](const std::string& msg) {
		lastError = msg;
		if (options.onError) options.onError(msg);
		TextGenResult errResult{};
		if (options.onDone) options.onDone(errResult);
	};

	if (options.onGenerationStart) options.onGenerationStart();

	auto res = http.PostStream("/chat/completions", requestStr, "application/json", headers,
		[&](const char* data, size_t len) -> bool {
			if (errBuf.size() < 4096) {
				size_t room = 4096 - errBuf.size();
				errBuf.append(data, len < room ? len : room);
			}
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

					// Some gateways return error objects (OpenAI {"error": ...} or
					// FastAPI {"detail": [...]}) instead of a choices chunk; surface
					// them rather than silently swallowing the missing "choices".
					if (json.is_object() && !json.contains("choices")
						&& (json.contains("error") || json.contains("detail") || json.contains("message"))) {
						if (errMsg.empty())
							errMsg = extractApiErrorMessage(payload, 200);
						return true;
					}

					if (!json.contains("choices") || !json["choices"].is_array()) continue;

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

	if (res.error != httplib::Error::Success) {
		fail("Connection error: " + res.error_str);
		return;
	}

	if (res.status != 200) {
		fail(extractApiErrorMessage(errBuf, res.status));
		return;
	}

	if (!errMsg.empty()) {
		fail(errMsg);
		return;
	}

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
