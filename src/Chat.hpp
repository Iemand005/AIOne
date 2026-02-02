#pragma once

#include <vector>
#include <functional>
#include <fstream>
#include <chrono>
#include <sstream>
#include <ctime>

#include <nlohmann/json.hpp>

#include "Message.hpp"
// #include "Role.h"
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
        // messages.push_back({Role::System, systemPrompt});
        timestamps.start();

        this->setSystemPrompt(systemPrompt);
    }

  

  size_t addMessage(Role role, std::string message, bool save = true) {
    return addMessage(Message(role, message), save);
  }

  size_t addMessage(std::string role, std::string message, bool save = true) {
    return addMessage(Message(role, message), save);
  }

  size_t addMessage(Message message, bool save = true) {
    uint64_t parentId = messages[messages.size() - 1].id;
    message.parentId = parentId;
    messages.push_back(message);
    
    timestamps.update();

    if (save) saveMessage(message);

    return messages.size() - 1;
  }

  void updateAt(size_t index, std::string newContent, bool save = true) {
    messages[index].content = newContent;
    messages[index].timestamps.update();

    if (save) saveMessage(messages[index]);
  }

  void setSystemPrompt(std::string prompt) {
    if (!messages.empty()) messages[0].content = prompt;
    else this->addMessage(Role::System, prompt);
  }

  std::vector<Message> getMessages() {
    return messages;
  }

  TextGenerationOptions getOptions() {
    return options;
  }

  void saveMessage(Message message) {
    std::time_t time = timestamps.creationTime / 1000;
    std::stringstream ss;
    std::tm* tm = std::localtime(&time);
    ss << "Chat_at_" << std::put_time(tm, "%Y%m%d_%H%M%S") << "_" << std::setfill('0') << std::setw(3) << (timestamps.creationTime % 1000) << ".jsonl";; ;;;;;;;;;;;
    std::string fileName = ss.str();

    std::ofstream(fileName, std::ios::app) << nlohmann::json{{
      {"id", message.id},
      {"parentId", message.parentId},
      {"creationTime", message.timestamps.creationTime},
      {"finishTime", message.timestamps.modificationTime},
      {"role", message.role},
      {"content", message.content}
    }}.dump() << '\n'; 
  }
};
