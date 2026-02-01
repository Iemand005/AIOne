
#include "ModelFactory.hpp"
#include "Chat.hpp"


class ChatManager {

  std::unique_ptr<ModelFactory> factory = std::make_unique<ModelFactory>();
  std::vector<std::shared_ptr<Chat>> chats;
  std::shared_ptr<Chat> currentChat;
  LLModel *model;

  std::string roleToString(Role role) {
    switch (role) {
      case Role::System: return "system";
      case Role::Assistant: return "assistant";
      case Role::User: return "user";
    }
  }

  using OnToken = std::function<void(std::string token)>;

  void sendAsync(std::string message, OnToken callback) {
    model->generateAsync()
  }

  void sendAsAsync(std::string message, Role role, OnToken callback) {
    
  }

  void sendAsAsync(std::string message, std::string role, OnToken callback) {
    
  }

  void setModel(LLModel *model) {
    this->model = model;
  }

};