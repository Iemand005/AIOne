#pragma once

#include <chrono>
#include <ctime>
#include <fstream>
#include <functional>
#include <iostream>
#include <mutex>
#include <sstream>
#include <vector>

#include "AIOneAPI.hpp"
#include "Message.hpp"
// #include "Role.h"
#include "TextContext.hpp"
#include "TextGenOptions.hpp"
#include "Timeable.hpp"

#include <nlohmann/json.hpp>

class AIONE_API Chat {
  unsigned long id = 0;
  std::vector<Message> messages = std::vector<Message>();
  TextGenOptions options = {};
  Timestamps timestamps;
  std::unique_ptr<std::mutex> messagesMutex = std::make_unique<std::mutex>();

  std::shared_ptr<TextContext> context;

  std::string m_chatFolder;
  std::string m_model;

  size_t addMessageNoLock(Role role, std::string message, bool save = true) { return addMessageNoLock(Message(role, message), save); }

  size_t addMessageNoLock(std::string role, std::string message, bool save = true) { return addMessageNoLock(Message(role, message), save); }

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

  Chat(std::shared_ptr<TextContext> newContext) { context = newContext; }

  ~Chat() = default;
  Chat(Chat&&) = default;
  Chat& operator=(Chat&&) = default;

  size_t addMessage(Role role, std::string message, bool save = true) {
    std::lock_guard<std::mutex> lock(*messagesMutex);
    return addMessageNoLock(Message(role, message), save);
  }

  size_t addMessage(std::string role, std::string message, bool save = true) {
    std::lock_guard<std::mutex> lock(*messagesMutex);
    return addMessageNoLock(Message(role, message), save);
  }

  Message getLastMessage() {
    std::lock_guard<std::mutex> lock(*messagesMutex);
    if (messages.size() < 1) return {};
    return messages[messages.size() - 1];
  }

  size_t addMessage(Message message, bool save = true) {
    std::lock_guard<std::mutex> lock(*messagesMutex);
    return addMessageNoLock(message, save);
  }

  Message createAndAddEmptyMessage(Role role) {
    auto message = Message(role);
    addMessage(message, false);
    return message;
  }

  void updateAt(size_t index, std::string newContent, bool save = true) {
    std::lock_guard<std::mutex> lock(*messagesMutex);
    messages[index].content = newContent;
    messages[index].timestamps.update();

    if (save) saveMessage(messages[index]);
  }

  void setSystemPrompt(std::string prompt) {
    std::lock_guard<std::mutex> lock(*messagesMutex);
    std::cout << "[DEBUG] addMessage called. 1 Current size: " << messages.size() << " Thread ID: " << std::this_thread::get_id() << std::endl;
    if (!messages.empty())
      messages[0].content = prompt;
    else
      this->addMessageNoLock(Role::System, prompt);
    std::cout << "[DEBUG] addMessage called. 2 Current size: " << messages.size() << " Thread ID: " << std::this_thread::get_id() << std::endl;
  }

  std::vector<Message> getMessages() {
    std::lock_guard<std::mutex> lock(*messagesMutex);
    return messages;
  }

  TextGenOptions* getOptions() { return &options; }

  void saveMessage(Message& message);

  void setModel(const std::string& model) { m_model = model; }
  const std::string& model() const { return m_model; }

  void setFolder(const std::string& folder) { m_chatFolder = folder; }
  const std::string& folder() const { return m_chatFolder; }

  void setMessages(std::vector<Message> msgs) {
    std::lock_guard<std::mutex> lock(*messagesMutex);
    messages = std::move(msgs);
    if (!messages.empty())
      timestamps = messages.back().timestamps;
  }

  nlohmann::json toJson() const;
  static Chat fromJson(const nlohmann::json& j);

  const Timestamps& timestampsRef() const { return timestamps; }
};
