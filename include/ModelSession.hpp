#pragma once

#include <memory>
#include <string>

#include "LLModel.hpp"
#include "ModelFactory.hpp"

namespace AIOne {

	class ModelSession {
	public:
		void loadModel(const std::string& path);
		std::string generate(const std::string& prompt, unsigned int maxTokens = 128);
		void unloadModel();

		bool isLoaded() const;

	private:
		ModelFactory factory;
		LLModelPtr model;
		std::shared_ptr<TextContext> context;
	};

}