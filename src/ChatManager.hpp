
#include "ModelFactory.hpp"
#include "Chat.hpp"

class ChatManager {

  std::unique_ptr<ModelFactory> factory = std::make_unique<ModelFactory>();
  std::vector<std::shared_ptr<Chat>> chats;
  std::shared_ptr<Chat> currentChat;
  LLModel *model;

  using OnToken = std::function<void(std::string token)>;

  void sendAsync(std::string message, OnToken callback) {
    
  }

  void setModel(LLModel *model) {
    this->model = model;
  }

};