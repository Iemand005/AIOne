#pragma once

#include <codecvt>
#include <iostream>
#include <locale>
#include <sstream>
#include <string>

#include "Chat.hpp"
#include "ChatStorage.hpp"
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
        if (!thinking) needsNewLineTrim = true;
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

  void regenerateAsync(uint64_t parentId, const AsyncTextGenOptions& options = {}, bool sanitizeReasoning = true) {
    AsyncTextGenOptions newOptions = options;
    newOptions.onDone = [this, options, parentId](const TextGenResult& output) {
      Message msg = output.output;
      msg.parentId = parentId;
      currentChat->addMessage(msg);
      if (options.onDone) options.onDone(output);
    };
    if (sanitizeReasoning) {
      newOptions.onThinkStateChange = [this, options](bool thinking) {
        if (!thinking) needsNewLineTrim = true;
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
    auto contextMsgs = currentChat->getMessageChain(parentId);
    if (provider) {
      auto msgsCopy = contextMsgs;
      std::thread([this, msgsCopy, newOptions]() {
        provider->complete(selectedModel, msgsCopy, newOptions);
      }).detach();
    } else {
      std::string prompt = model->chatToPrompt(contextMsgs);
      model->generateAsync(prompt, newOptions);
    }
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
  void setModel(const std::string& model) {
    selectedModel = model;
    if (currentChat) currentChat->setModel(model);
  }

  void setChat(std::shared_ptr<Chat> chat) { currentChat = chat; }

  void setSystemPrompt(std::string prompt) {
    currentChat->setSystemPrompt(prompt);
    saveCurrentChatMetadata();
  }

  TextGenOptions* currentChatOptions() { return currentChat->getOptions(); }

  Chat* getCurrentChat() { return currentChat.get(); }

  // Storage

  void setStoragePath(const std::string& path) { m_storagePath = path; }

  void initStorage() {
    if (m_storagePath.empty()) m_storagePath = ChatStorage::rootPath();
    ChatStorage::init();
  }

  void createNewChat(const std::string& title, const std::string& modelName = "",
                     const std::string& systemPrompt = "",
                     const TextGenOptionsBase& params = {}) {
    ChatMetadata meta;
    meta.title = title;
    meta.model = modelName;
    meta.systemPrompt = systemPrompt;
    meta.params = params;

    std::string folder = ChatStorage::createChat(meta);
    auto chat = std::make_shared<Chat>(systemPrompt);
    chat->setFolder(ChatStorage::rootPath() + "/" + folder);
    chat->setModel(modelName);
    currentChat = chat;
  }

  void loadChat(const std::string& folder) {
    std::string fullPath = (m_storagePath.empty() ? ChatStorage::rootPath() : m_storagePath) + "/" + folder;
    auto meta = ChatStorage::loadMetadata(folder);
    auto msgs = ChatStorage::loadMessages(folder);

    auto chat = std::make_shared<Chat>();
    if (!msgs.empty() && msgs[0].role == "system") {
      chat->setSystemPrompt(msgs[0].content);
    } else if (!meta.systemPrompt.empty()) {
      chat->setSystemPrompt(meta.systemPrompt);
    }
    chat->setFolder(fullPath);
    chat->setModel(meta.model);
    chat->setMessages(std::move(msgs));
    currentChat = chat;
  }

  void saveCurrentChatMetadata() {
    if (!currentChat || currentChat->folder().empty()) return;

    ChatMetadata meta;
    meta.folder = currentChat->folder();
    meta.title = guessChatTitle();
    meta.model = currentChat->model();
    auto chatMsgs = currentChat->getMessages();
    meta.systemPrompt = (!chatMsgs.empty() && chatMsgs[0].role == "system") ? chatMsgs[0].content : "";
    meta.params = *currentChat->getOptions();
    meta.updated = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    // Extract folder name from full path
    std::string folderName = currentChat->folder();
    auto pos = folderName.rfind('/');
    if (pos != std::string::npos)
      folderName = folderName.substr(pos + 1);
    pos = folderName.rfind('\\');
    if (pos != std::string::npos)
      folderName = folderName.substr(pos + 1);

    ChatStorage::saveMetadata(folderName, meta);
  }

  std::vector<ChatMetadata> listChats() {
    return ChatStorage::scan();
  }

  void exportCurrentChat(const std::string& exportPath) {
    if (!currentChat || currentChat->folder().empty()) return;
    std::string folderName = currentChat->folder();
    auto pos = folderName.rfind('/');
    if (pos != std::string::npos)
      folderName = folderName.substr(pos + 1);
    pos = folderName.rfind('\\');
    if (pos != std::string::npos)
      folderName = folderName.substr(pos + 1);
    ChatStorage::exportChat(folderName, exportPath);
  }

  std::string selectedModelName() { return selectedModel; }

private:

    ILLMProvider* provider = nullptr;
	std::string selectedModel;

    std::string m_storagePath;

    std::string guessChatTitle() {
        if (!currentChat) return "Untitled";
        auto msgs = currentChat->getMessages();
        // Find first user message
        for (const auto& m : msgs) {
            if (m.role == "user") {
                std::string title = m.content;
                if (title.size() > 60) title = title.substr(0, 60) + "...";
                // Remove newlines
                for (size_t i = 0; i < title.size(); ++i)
                    if (title[i] == '\n' || title[i] == '\r') title[i] = ' ';
                return title;
            }
        }
        return "Untitled";
    }
};

typedef std::unique_ptr<ChatManager> ChatManagerPtr;