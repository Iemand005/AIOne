#pragma once

#include <codecvt>
#include <iostream>
#include <locale>
#include <sstream>
#include <string>

#include "Chat.hpp"
#include "ModelFactory.hpp"

#include "./Providers/ILLMProvider.hpp"
#include "Role.h"

using namespace AIOne;

class ChatManager {
	std::unique_ptr<ModelFactory> factory;
	std::vector<std::shared_ptr<Chat>> chats;
	std::shared_ptr<Chat> currentChat;
	LLModel* model;
	Role userRole = Role::User;  // if user wants to switch role I guess
	bool needsNewLineTrim = false;



 public:

	ChatManager(ILLMProvider& provider) : provider(&provider) {
		currentChat = std::make_shared<Chat>();
	}

  ChatManager(LLModel* model, std::string systemPrompt = "") : model(model) {
    factory = std::make_unique<ModelFactory>();
    chats = std::vector<std::shared_ptr<Chat>>();
    currentChat = std::make_shared<Chat>(model->createContext());
  };

  void sendAsync(std::wstring message, const AsyncTextGenOptions& options = {}) {
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    sendAsync(converter.to_bytes(message), options);
  }

  std::string trimLeadingNewlines(const std::string& s) {
    size_t start = 0;
    while (start < s.size() && s[start] == '\n') ++start;
    return s.substr(start);
  }

  void sendAsync(std::string message, const AsyncTextGenOptions& options = {}, bool sanitizeReasoning = true) {
    AsyncTextGenOptions newOptions = options;
    newOptions.onDone = [this, options](const TextGenResult& output) {
      currentChat->addMessage(output.output);
      if (options.onDone) options.onDone(output);
    };
    if (sanitizeReasoning) {
      newOptions.onThinkStateChange = [this, options](bool thinking) {
        if (!thinking) needsNewLineTrim = true;  // Thinking ended, cut out the newlines that come inbetween this and the first word.
        if (options.onThinkStateChange) options.onThinkStateChange(thinking);
      };
      newOptions.onToken = [this, options](const std::string& token) {
        auto trimmedToken = token;
        if (needsNewLineTrim) trimmedToken = trimLeadingNewlines(token);
        if (trimmedToken.empty()) return;
        needsNewLineTrim = false;
        if (options.onToken) options.onToken(trimmedToken);
      };
    }
    if (!newOptions.systemPrompt.empty()) setSystemPrompt(newOptions.systemPrompt);
    sendAsAsync(message, userRole, newOptions);
  }

	void sendAsAsync(std::string message, Role role, const AsyncTextGenOptions& options = {}) {
		sendAsAsync(message, RoleClass::toString(role), options);
	}

	void sendAsAsync(std::string message, std::string role, const AsyncTextGenOptions& options = {}) {
	currentChat->addMessage(role, message);
	if (provider) {
		auto msgCopy = currentChat->getMessages();
		std::thread([this, msgCopy, options]() {
			provider->complete(selectedModel, msgCopy, options);
		}).detach();
	}
	else model->generateAsync(currentChat, options);
	}

	void completeAsync(std::string message, const AsyncTextGenOptions& options = {}) {
    // model->generateAsync(currentChat, Message(userRole, message), options);
    completeAsAsync(message, RoleClass::toString( userRole), options);
	}

	void completeAsAsync(std::string message, std::string role, const AsyncTextGenOptions& options = {}) {
		if (provider) {
			auto msgCopy = currentChat->getMessages();
			msgCopy.push_back(Message(role, message));
			std::thread([this, msgCopy, options]() {
				provider->complete(selectedModel, msgCopy, options);
			}).detach();
		}
		else {
			model->generateAsync(currentChat, Message(role, message), options);
		}
	}

  void setModel(LLModel* model) { this->model = model; }
  void setModel(const std::string& model) { selectedModel = model; }

  void setChat(std::shared_ptr<Chat> chat) { currentChat = chat; }

  void setSystemPrompt(std::string prompt) { currentChat->setSystemPrompt(prompt); }

  TextGenOptions* currentChatOptions() { return currentChat->getOptions(); }

private:

    ILLMProvider* provider = nullptr;
	std::string selectedModel;
};

typedef std::unique_ptr<ChatManager> ChatManagerPtr;