#pragma once

#include "ILLMProvider.hpp"

#include "..//HttpClient.hpp"

namespace AIOne {
	class OpenAIProvider : public ILLMProvider {
	public:
		OpenAIProvider(std::string baseUrl, std::string apiKey);

		std::vector<ModelInfo> getModels() override;

		void complete(const std::string& model, const std::vector<Message>& messages, const AsyncTextGenOptions& options) override;

		const std::string& getLastError() const { return lastError; }

	private:
		HttpClient http;
		std::string  apiKey;
		std::string  lastError;

	};
}