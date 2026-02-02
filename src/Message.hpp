#pragma once

#include <string>

#include "Timeable.hpp"

class Message {
public:
  unsigned long long id, parentId;
  std::string role;
  std::string content;
  Timestamps timestamps;

  Message(std::string role, std::string content) : role(role), content(content) {
    this->timestamps.start();
  }

  void finished() {
    this->timestamps.finish();
  }
};
