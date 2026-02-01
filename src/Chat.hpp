#pragma once

#include <vector>
#include <functional>

#include "Message.hpp"
#include "Role.h"

class Chat {
    std::vector<Message> messages = std::vector<Message>();
    TextGenerationOptions options{};

public:

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

  std::vector<Message> getMessages() {
    return messages;
  }

  TextGenerationOptions getOptions() {
    return options;
  }
};
