#pragma once

#include <string>
#include <iostream>

#include "Role.h"
#include "Timeable.hpp"

struct Message { // TODO: erename to chatmessage? just to be clear
  unsigned long long id, parentId;
  std::string role; // TODO  Store enum value instead and convert when setting? but I left it string to ssupport any custo role in case needed..
  std::string content;
  Timestamps timestamps;

  Message(const Message& other) = default;
  Message(Message&& other) noexcept = default;
  Message& operator=(const Message& other) = default;
  Message& operator=(Message&& other) noexcept = default;

  Message(uint64_t parentId = 0) {
      this->timestamps.start();
      this->id = timestamps.randomId();
      this->parentId = parentId;
  }

  Message(std::string role, std::string content, uint64_t parentId = 0) : Message(parentId) {
      this->parentId = parentId;
      this->role = role;
      this->content = content;
    }
  
    Message(Role role, std::string content = "", uint64_t parentId = 0) : Message(RoleClass::toString(role), content, parentId) {}

  ~Message() {
        std::cout << "[DEBUG] Message destructor called for id: " << id 
                  << ", role: " << role 
                  << ", content size: " << content.size() 
                  << std::endl;
    }

  void finished() { this->timestamps.update(); }
};
