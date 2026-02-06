#pragma once

#include "ModelFactory.hpp"
#include "ChatManager.hpp"

class ModelManager {

  ModelFactoryPtr factory;

  ChatManagerPtr chatManager;

  LLModelPtr llm;

public:
  ModelManager() : factory(std::make_unique<ModelFactory>()) {}

  void loadLLMAsync(std::string path, LLModelOptions options = {}) {
    factory->loadLLMAsync(path, options, [this](LLModelPtr model) {
      llm = std::move(model);
      chatManager = std::make_unique<ChatManager>(getLLM());
    });
  }

  LLModel *getLLM() { return llm.get(); }
  ChatManager *getChatManager() {return chatManager.get(); }

};

typedef std::unique_ptr<ModelManager> ModelManagerPtr;