#pragma once

#include <string>

#include "Role.h"
#include "Timeable.hpp"

class Message {
public:
  unsigned long long id, parentId;
  std::string role;
  std::string content;
  Timestamps timestamps;

  Message(Role role, std::string &content) : Message(roleToString(role), content) {}

  Message(std::string role, std::string content) : role(role), content(content) {
    this->timestamps.start();
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
