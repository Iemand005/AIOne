#pragma once

#include <vector>
#include <functional>
#include <fstream>
#include <chrono>
#include <sstream>
#include <ctime>
#include <iostream>
#include <mutex>


#include "AIOneAPI.hpp"
#include "Message.hpp"
// #include "Role.h"
#include "Timeable.hpp"
#include "TextContext.hpp"
#include "TextGenOptions.hpp"

class AIONE_API Chat {
    unsigned long id = 0;
    std::vector<Message> messages = std::vector<Message>();
    TextGenOptions options = {};
    Timestamps timestamps;
    std::mutex messagesMutex;

    std::shared_ptr<TextContext> context;

    size_t addMessageNoLock(Role role, std::string message, bool save = true) {
        return addMessageNoLock(Message(role, message), save);
    }

    size_t addMessageNoLock(std::string role, std::string message, bool save = true) {
        return addMessageNoLock(Message(role, message), save);
    }

    size_t addMessageNoLock(Message message, bool save = true) {
        if (messages.size() > 0) {
            uint64_t parentId = messages[messages.size() - 1].id;
            message.parentId = parentId;
        }

        messages.push_back(message);

        timestamps.update();

        if (save) saveMessage(message);

        return messages.size() - 1;
    }

public:
    Chat(std::string systemPrompt = "") {        
        messages = std::vector<Message>();
        
        timestamps.start();

        this->setSystemPrompt(systemPrompt);
    }

    Chat(std::shared_ptr<TextContext> newContext) {
      context = newContext;
    }

    ~Chat() {

    }

  size_t addMessage(Role role, std::string message, bool save = true) {
      std::lock_guard<std::mutex> lock(messagesMutex);
    return addMessageNoLock(Message(role, message), save);
  }

  size_t addMessage(std::string role, std::string message, bool save = true) {
      std::lock_guard<std::mutex> lock(messagesMutex);
    return addMessageNoLock(Message(role, message), save);
  }

  Message getLastMessage() {
    std::lock_guard<std::mutex> lock(messagesMutex);
    if (messages.size() < 1) return {};
    return messages[messages.size() -1];
  }

  size_t addMessage(Message message, bool save = true) {
    std::lock_guard<std::mutex> lock(messagesMutex);
      return addMessageNoLock(message, save);
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
      std::lock_guard<std::mutex> lock(messagesMutex);
    std::cout << "[DEBUG] addMessage called. 1 Current size: " << messages.size() << " Thread ID: " << std::this_thread::get_id() << std::endl;
    if (!messages.empty())
        messages[0].content = prompt;
    else this->addMessageNoLock(Role::System, prompt);
    std::cout << "[DEBUG] addMessage called. 2 Current size: " << messages.size() << " Thread ID: " << std::this_thread::get_id() << std::endl;
  }

  std::vector<Message> getMessages() {
    std::lock_guard<std::mutex> lock(messagesMutex);
    return messages;
  }

  TextGenOptions *getOptions() {
    return &options;
  }

  void saveMessage(Message &message);
};
