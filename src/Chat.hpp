#pragma once

#include <vector>
#include <functional>
#include <fstream>
#include <chrono>

#include <nlohmann/json.hpp>

#include "Message.hpp"
#include "Role.h"
#include "Timeable.hpp"

class Chat : public Timeable {
    std::vector<Message> messages;
    TextGenerationOptions options;

public:

    

    Chat() : Chat("") {}

    Chat(std::string systemPrompt) {
        messages = std::vector<Message>();
        messages.push_back({roleToString(Role::System), systemPrompt});
    }

  std::string roleToString(Role role) {
    switch (role) {
      case Role::System: return "system";
      case Role::Assistant: return "assistant";
      case Role::User: return "user";
    }
  }

  void addMessage(std::string message, Role role) {
    addMessage(message, roleToString(role));
  }

  void addMessage(std::string message, std::string role) {
    messages.push_back({role, message});
  }

  void setSystemPrompt(std::string prompt) {
    messages[0] = {roleToString(Role::System), prompt};
      // messages.push_back({roleToString(Role::System), prompt});
  }

  std::vector<Message> getMessages() {
    return messages;
  }

  TextGenerationOptions getOptions() {
    return options;
  }

  void createSaveFile() {
    std::string fileName = "chat.jsonl";
    currentTi
    std::ofstream(fileName, std::ios::app) << nlohmann::json{{"text", msg}}.dump() << '\n'; 
  }
};
