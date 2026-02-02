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

  ChatManager(LLModel *model, std::string systemPrompt = "") : model(model) {
      factory = std::make_unique<ModelFactory>();
      chats = std::vector<std::shared_ptr<Chat>>();
      currentChat = std::make_shared<Chat>(systemPrompt);
  };

  void sendAsync(std::string message, FinishCallback onDone, TokenCallback onToken, ProgressCallback onInputEval) {
    sendAsAsync(message, userRole, onDone, onToken, onInputEval);
  }

  void sendAsAsync(std::string message, Role role, FinishCallback onDone, TokenCallback onToken, ProgressCallback onInputEval) {
      currentChat->addMessage(role, message);
      model->generateAsync(currentChat, onDone, onToken, onInputEval);
  }

  void sendAsAsync(std::string message, std::string role, FinishCallback onDone, TokenCallback onToken, ProgressCallback onInputEval) {
    currentChat->addMessage(role, message);
    model->generateAsync(currentChat, onDone, onToken, onInputEval);
  }

  void completeAsync(std::string message, FinishCallback onDone, TokenCallback onToken, ProgressCallback onInputEval) {
    model->generateAsync(currentChat, Message(userRole, message), onDone, onToken, onInputEval);
  }

  void completeAsAsync(std::string message, std::string role, FinishCallback onDone, TokenCallback onToken, ProgressCallback onInputEval) {
    // auto last = currentChat->getLastMessage();
    // if (last.role != role) last = currentChat->createAndAddEmptyMessage(role);
    // currentChat->addMessage(role, message);
    model->generateAsync(currentChat, Message(role, message), onDone, onToken, onInputEval);
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

};
