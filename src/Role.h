#pragma once

#include <string>

enum class Role {
  System,
  Assistant,
  User
};

class RoleClass {
public:
  
  static std::string toString(Role role) {
    switch (role) {
      case Role::System: return "system";
      case Role::Assistant: return "assistant";
      case Role::User: return "user";
      default: return "unknown";
    }
  }
};