
#include "ModelFactory.hpp"
#include "Chat.hpp"


class ChatManager {

  std::unique_ptr<ModelFactory> factory = std::make_unique<ModelFactory>();
  std::vector<std::shared_ptr<Chat>> chats;
  std::shared_ptr<Chat> currentChat;
  LLModel *model;
  Role userRole = Role::User; // if user wants to switch role I guess

  

  using OnToken = std::function<void(std::string token)>;

  void sendAsync(std::string message, TokenCallback onToken, InputEvalCallback onInputEval) {
    sendAsAsync(message, userRole, onToken, onInputEval);
    // currentChat->addMessage(message, userRole);
  }

  void sendAsAsync(std::string message, Role role, TokenCallback onToken, InputEvalCallback onInputEval) {
    sendAsAsync(message, currentChat->roleToString(role), onToken, onInputEval);
  }

  void sendAsAsync(std::string message, std::string role, TokenCallback onToken, InputEvalCallback onInputEval) {
    // Message chatMessage = {role, message};
    // chatMessages->push_back(chatMessage);//dd
    currentChat->addMessage(message, role);
    model->generateAsync(currentChat.get(), onToken, onInputEval, currentChat->getOptions());
  }

  void setModel(LLModel *model) {
    this->model = model;
  }

  void setChat(std::shared_ptr<Chat> chat) {
    currentChat = chat;
  }

};