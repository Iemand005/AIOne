#pragma once

#include <memory>
#include <string>

#include "Callbacks.h"
#include "LLModel.hpp"
#include "ModelFactory.hpp"

namespace AIOne {

	struct GenerateCallbacks {
		TokenCallback onToken = nullptr;
		TokenReasoningCallback onTokenReasoning = nullptr;
		ProgressCallback onInputEval = nullptr;
		NotifyCallback onGenerationStart = nullptr;
		ThinkStateChangedCallback onThinkStateChange = nullptr;
	};

	class ModelSession {
	public:
		bool loadModel(
			const std::string& path,
			const ProgressCallback& onProgress = nullptr
		);
		TextGenResult generate(
			const std::string& prompt,
			unsigned int maxTokens = 128,
			const GenerateCallbacks& callbacks = {}
		);
		void unloadModel();

		bool isLoaded() const;

	private:
		ModelFactory factory;
		LLModelPtr model;
		std::shared_ptr<TextContext> context;
	};

}