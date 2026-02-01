
#include "ModelFactory.hpp"
#include "Chat.hpp"

class ModelManager {

  std::unique_ptr<ModelFactory> factory = std::make_unique<ModelFactory>();
  std::vector<Chat> chats;

  using OnToken = std::function<void(std::string token)>;

  void sendAsync(std::string message, OnToken callback) {
    
  }

};