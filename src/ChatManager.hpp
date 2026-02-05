#pragma once

#include "ModelFactory.hpp"
#include "Chat.hpp"


class ChatManager {

  std::unique_ptr<ModelFactory> factory;
    std::vector<std::shared_ptr<Chat>> chats;
  std::shared_ptr<Chat> currentChat;
  LLModel *model;
  Role userRole = Role::User; // if user wants to switch role I guess

public:

  ChatManager(LLModel *model) : ChatManager(model, "") {}

  ChatManager(LLModel *model, std::string systemPrompt) : model(model) {
      factory = std::make_unique<ModelFactory>();
      chats = std::vector<std::shared_ptr<Chat>>();
      currentChat = std::make_shared<Chat>(systemPrompt);
  };

  void sendAsync(std::string message, const AsyncTextGenOptions& options = {}) {
      AsyncTextGenOptions newOptions = options;
      newOptions.onDone = [this, options](const TextGenResult &output) {
          currentChat->addMessage(output.output);
          if (options.onDone) options.onDone(output);
      };
      sendAsAsync(message, userRole, newOptions);
  }

  void sendAsAsync(std::string message, Role role, const AsyncTextGenOptions& options = {}) {
      currentChat->addMessage(role, message);
      model->generateAsync(currentChat, options);
  }

  void sendAsAsync(std::string message, std::string role, const AsyncTextGenOptions& options = {}) {
    currentChat->addMessage(role, message);
    model->generateAsync(currentChat, options);
  }

  void completeAsync(std::string message, const AsyncTextGenOptions& options = {}) {
    model->generateAsync(currentChat, Message(userRole, message), options);
  }

  void completeAsAsync(std::string message, std::string role, const AsyncTextGenOptions& options = {}) {
    // auto last = currentChat->getLastMessage();
    // if (last.role != role) last = currentChat->createAndAddEmptyMessage(role);
    // currentChat->addMessage(role, message);
    model->generateAsync(currentChat, Message(role, message), options);
  }

  void setModel(LLModel *model) {
    this->model = model;
  }

  void setChat(std::shared_ptr<Chat> chat) {
    currentChat = chat;
  }

  void setSystemPrompt(std::string prompt) {
      currentChat->setSystemPrompt(prompt);
  }

  TextGenOptions *currentChatOptions() {
      return currentChat->getOptions();
  }

};

typedef std::unique_ptr<ChatManager> ChatManagerPtr;