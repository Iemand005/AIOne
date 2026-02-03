#pragma once

#include <string>
#include <iostream>

#include "Role.h"
#include "Timeable.hpp"

struct Message {
  unsigned long long id, parentId;
  std::string role;
  std::string content;
  Timestamps timestamps;

  Message(const Message& other) = default;
Message(Message&& other) noexcept = default;
Message& operator=(const Message& other) = default;
Message& operator=(Message&& other) noexcept = default;

  Message() {
      this->timestamps.start();
      this->id = timestamps.randomId();
  }

  Message(Role role, std::string content = "", uint64_t parentId = 0) 
    : id(Timestamps().randomId()), 
      parentId(parentId), 
      role(roleToString(role)),
      content(content),
      timestamps() {
    this->timestamps.start();
}

Message(std::string role, std::string content, uint64_t parentId = 0) 
    : id(Timestamps().randomId()),
      parentId(parentId), 
      role(role), 
      content(content),
      timestamps() {
    this->timestamps.start();
}
  ~Message() {
        std::cout << "[DEBUG] Message destructor called for id: " << id 
                  << ", role: " << role 
                  << ", content size: " << content.size() 
                  << std::endl;
    }
  std::string roleToString(Role role) {
    switch (role) {
      case Role::System: return "system";
      case Role::Assistant: return "assistant";
      case Role::User: return "user";
    }
  }

  void finished() {
    this->timestamps.update();
  }
};
