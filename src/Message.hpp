#pragma once

#include <string>

#include "Role.h"
#include "Timeable.hpp"

struct Message {
  unsigned long long id, parentId;
  std::string role;
  std::string content;
  Timestamps timestamps;

  Message() {
      this->timestamps.start();
      this->id = timestamps.randomId();
  }

  Message(Role role, std::string content = "", uint64_t parentId = 0) : parentId(parentId), content(content) {
      this->role = roleToString(role);
  }

  Message(std::string role, std::string content, uint64_t parentId = 0) : parentId(parentId), role(role), content(content) {}

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
