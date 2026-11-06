#pragma once

#include <memory>
#include <string>
#include <vector>

#include "Callbacks.h"
#include "LLModel.hpp"
#include "Message.hpp"
#include "ModelFactory.hpp"
#include "TextGenOptions.hpp"

namespace AIOne {

	using GenerateOptions = TextGenOptionsBase;

	struct GenerateCallbacks {
		TokenCallback onToken = nullptr;
		TokenReasoningCallback onTokenReasoning = nullptr;
		ProgressCallback onInputEval = nullptr;
		NotifyCallback onGenerationStart = nullptr;
		ThinkStateChangedCallback onThinkStateChange = nullptr;
		std::function<bool()> shouldStop;
	};

	class ModelSession {
	public:
		bool loadModel(
			const std::string& path,
			const ProgressCallback& onProgress = nullptr
		);
		bool loadModelWithOptions(
			const std::string& path,
			bool noHost,
			const ProgressCallback& onProgress = nullptr
		);
		TextGenResult generate(
			const std::string& prompt,
			unsigned int maxTokens = 128,
			const GenerateCallbacks& callbacks = {}
		);
		TextGenResult generateWithOptions(
			const std::string& prompt,
			const GenerateOptions& options,
			const GenerateCallbacks& callbacks = {}
		);
		std::string formatChatPrompt(
			const std::vector<Message>& messages,
			bool addGenerationPrompt = true
		);
		std::string formatChatDraftPrompt(
			const std::vector<Message>& messages,
			Message draft
		);
		void unloadModel();

		bool isLoaded() const;

	private:
		ModelFactory factory;
		LLModelPtr model;
		std::shared_ptr<TextContext> context;
	};

}