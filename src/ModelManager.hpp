#pragma once

#include "ModelFactory.hpp"

class ModelManager {

  ModelFactoryPtr factory;

  LLModelPtr llm;

public:
  ModelManager() {
    factory = std::make_unique<ModelFactory>();
  }

  void loadLLMAsync(std::string path, LLModelOptions options = {}) {
    factory->loadLLMAsync(path, options, [this](LLModelPtr model) {
      llm = std::move(model);
    });
  }
};

typedef std::unique_ptr<ModelManager> ModelManagerPtr;