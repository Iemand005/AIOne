#pragma once

#include <vector>
#include <functional>
#include <fstream>
#include <chrono>
#include <sstream>
#include <ctime>
#include <iostream>
#include <mutex>

#include <nlohmann/json.hpp>

#include "Message.hpp"
// #include "Role.h"
#include "Timeable.hpp"

class Chat {
    unsigned long id = 0;
    std::vector<Message> messages = std::vector<Message>();
    TextGenerationOptions options = {};
    Timestamps timestamps;
    std::mutex messagesMutex;

public:
    Chat() : Chat("") {}

    Chat(std::string systemPrompt) {
        // messages;
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

  Message getLastMessage() {
    if (messages.size() < 1) return {};
    return messages[messages.size() -1];
  }

  size_t addMessage(Message message, bool save = true) {
    std::lock_guard<std::mutex> lock(messagesMutex);
      if (messages.size() > 0) {
          uint64_t parentId = messages[messages.size() - 1].id;
          message.parentId = parentId;
      }
      std::cerr << "I'm adding";
      std::cout << "[DEBUG] addMessage called. 3 Current size: " << messages.size() << " Thread ID: " << std::this_thread::get_id() << std::endl;
    messages.push_back(message);
    
    timestamps.update();

    if (save) saveMessage(message);

    std::cout << "[DEBUG] addMessage called. 4 Current size: " << messages.size() << " Thread ID: " << std::this_thread::get_id() << std::endl;

    return messages.size() - 1;
  }

  Message createAndAddEmptyMessage(Role role) {
    auto message = Message(role);
    addMessage(message, false);
    return message;
  }

  void updateAt(size_t index, std::string newContent, bool save = true) {
    std::lock_guard<std::mutex> lock(messagesMutex);
    messages[index].content = newContent;
    messages[index].timestamps.update();

    if (save) saveMessage(messages[index]);
  }

  void setSystemPrompt(std::string prompt) {
    std::cout << "[DEBUG] addMessage called. 1 Current size: " << messages.size() << " Thread ID: " << std::this_thread::get_id() << std::endl;
    if (!messages.empty())
        messages[0].content = prompt;
    else this->addMessage(Role::System, prompt);
    std::cout << "[DEBUG] addMessage called. 2 Current size: " << messages.size() << " Thread ID: " << std::this_thread::get_id() << std::endl;
  }

  std::vector<Message> &getMessages() {
    std::lock_guard<std::mutex> lock(messagesMutex);
    return messages;
  }

  TextGenerationOptions getOptions() {
    return options;
  }

  void saveMessage(Message &message) {
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
