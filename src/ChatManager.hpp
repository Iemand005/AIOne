
#include "ModelFactory.hpp"
#include "Chat.hpp"


class ChatManager {

  std::unique_ptr<ModelFactory> factory = std::make_unique<ModelFactory>();
  std::vector<std::shared_ptr<Chat>> chats;
  std::shared_ptr<Chat> currentChat;
  LLModel *model;
  Role userRole = Role::User; // if user wants to switch role I guess

public:

  void sendAsync(std::string message, FinishCallback onDone, TokenCallback onToken, InputEvalCallback onInputEval) {
    sendAsAsync(message, userRole, onDone, onToken, onInputEval);
  }

  void sendAsAsync(std::string message, Role role, FinishCallback onDone, TokenCallback onToken, InputEvalCallback onInputEval) {
    sendAsAsync(message, currentChat->roleToString(role), onDone, onToken, onInputEval);
  }

  void sendAsAsync(std::string message, std::string role, FinishCallback onDone, TokenCallback onToken, InputEvalCallback onInputEval) {
    currentChat->addMessage(message, role);
    model->generateAsync(currentChat.get(), onDone, onToken, onInputEval);
  }

  void setModel(LLModel *model) {
    this->model = model;
  }

  void setChat(std::shared_ptr<Chat> chat) {
    currentChat = chat;
  }

};
