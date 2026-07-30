#pragma once

#include <algorithm>
#include <chrono>
#include <ctime>
#include <fstream>
#include <functional>
#include <iostream>
#include <map>
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

  std::map<uint64_t, size_t> m_currentVersionIndex;

  size_t addMessageNoLock(Role role, std::string message, bool save = true) { return addMessageNoLock(Message(role, message), save); }

  size_t addMessageNoLock(std::string role, std::string message, bool save = true) { return addMessageNoLock(Message(role, message), save); }

  size_t addMessageNoLock(Message message, bool save = true) {
    if (messages.size() > 0 && message.parentId == 0) {
      message.parentId = messages[messages.size() - 1].id;
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
    if (!messages.empty()) {
      messages[0].role = "system";
      messages[0].content = prompt;
    } else {
      this->addMessageNoLock(Role::System, prompt);
    }
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

  // Version/branching support
  std::vector<Message> getSiblings(uint64_t parentId) {
    std::lock_guard<std::mutex> lock(*messagesMutex);
    std::vector<Message> result;
    for (const auto& msg : messages) {
      if (msg.parentId == parentId) result.push_back(msg);
    }
    std::sort(result.begin(), result.end(), [](const Message& a, const Message& b) {
      return a.timestamps.creationTime < b.timestamps.creationTime;
    });
    return result;
  }

  std::vector<Message> getActivePath() {
    std::lock_guard<std::mutex> lock(*messagesMutex);
    std::vector<Message> result;
    uint64_t pid = 0;
    while (true) {
      std::vector<Message> siblings;
      for (const auto& msg : messages) {
        if (msg.parentId == pid) siblings.push_back(msg);
      }
      if (siblings.empty()) break;
      std::sort(siblings.begin(), siblings.end(), [](const Message& a, const Message& b) {
        return a.timestamps.creationTime < b.timestamps.creationTime;
      });
      size_t idx = 0;
      auto it = m_currentVersionIndex.find(pid);
      if (it != m_currentVersionIndex.end()) idx = it->second;
      if (idx >= siblings.size()) idx = 0;
      result.push_back(siblings[idx]);
      pid = siblings[idx].id;
    }
    return result;
  }

  // Follows exact parentId chain from message upToId to root (no version selection)
  std::vector<Message> getMessageChain(uint64_t upToId) {
    std::lock_guard<std::mutex> lock(*messagesMutex);
    std::map<uint64_t, Message> byId;
    for (const auto& m : messages) byId[m.id] = m;

    std::vector<Message> result;
    uint64_t cur = upToId;
    while (cur != 0) {
      auto it = byId.find(cur);
      if (it == byId.end()) break;
      result.push_back(it->second);
      cur = it->second.parentId;
    }
    std::reverse(result.begin(), result.end());
    return result;
  }

  size_t getVersionCount(uint64_t parentId) {
    return getSiblings(parentId).size();
  }

  size_t getCurrentVersionIndex(uint64_t parentId) {
    auto it = m_currentVersionIndex.find(parentId);
    if (it == m_currentVersionIndex.end()) return 0;
    size_t count = getVersionCount(parentId);
    return count > 0 ? std::min(it->second, count - 1) : 0;
  }

  void setCurrentVersionIndex(uint64_t parentId, size_t index) {
    m_currentVersionIndex[parentId] = index;
  }
};
