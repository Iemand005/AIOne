#pragma once

#include <vector>
#include <functional>
#include <fstream>
#include <chrono>
#include <sstream>
#include <ctime>

#include <nlohmann/json.hpp>

#include "Message.hpp"
#include "Role.h"
#include "Timeable.hpp"

class Chat {
    unsigned long id;
    std::vector<Message> messages;
    TextGenerationOptions options;
    Timestamps timestamps;

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

  void saveMessage(Message message) {
    std::time_t time_sec = timestamps.creationTime / 1000;
    std::stringstream ss;
    ss << "Chat_at_" << std::put_time(time_sec, "%Y%m%d_%H%M%S") << "_" << std::setfill('0') << std::setw(3) << (timestamps.creationTime % 1000) << ".jsonl";; ;;;;;;;;;;;
    std::string fileName = ss.str();
    // std::string fileName = std::format()"chat.jsonl";
    std::ofstream(fileName, std::ios::app) << nlohmann::json{{
      {"id", message.id},
      {"parentId", message.parentId},
      {"creationTime", message.timestamps.creationTime},
      {"finishTime", message.timestamps.modificationTime}
      {"role", message.role},
      {"content", message.content}
    }}.dump() << '\n'; 
  }
};
