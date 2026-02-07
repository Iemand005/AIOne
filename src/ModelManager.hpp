#pragma once

#include <string>
#include <locale>
#include <codecvt>

#include "ModelFactory.hpp"
#include "ChatManager.hpp"

class ModelManager {

  ModelFactoryPtr factory;

  ChatManagerPtr chatManager;

  LLModelPtr llm;

public:
  ModelManager() : factory(std::make_unique<ModelFactory>()) {}

  void loadLLMAsync(std::wstring path, LLModelOptionsAsync options = {}) {
      std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
        loadLLMAsync(converter.to_bytes(path), options);
  }

  void loadLLMAsync(std::string path, LLModelOptionsAsync options = {}) {
      LLModelOptions& syncOptions = dynamic_cast<LLModelOptions&>(options);
    factory->loadLLMAsync(path, syncOptions, [this, options](LLModelPtr model) {
      llm = std::move(model);
      chatManager = std::make_unique<ChatManager>(getLLM());
      if (options.onDone) options.onDone();
    });
  }

  LLModel *getLLM() { return llm.get(); }
  ChatManager *getChatManager() {return chatManager.get(); }

};

typedef std::unique_ptr<ModelManager> ModelManagerPtr;