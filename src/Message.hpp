#pragma once

#include <string>
#include "Saveables.h"

class Message {
  long long id, parentId;
  std::string role;
  std::string content;
  Timestamps timestamps;

public:
  Message() {
    this->timestamps.creationTime = currentTimeMillis();
  }

  void finished() {
    this->timestamps.finishTime = currentTimeMillis();
  }

  char *toBytes() {
    size_t roleSize = role.size();
    size_t messageSize = content.size();
    auto message = allocateStruct<SaveableMessageHeader>(roleSize + messageSize);
    message->id = this->id;
    message->parentId = this->parentId;
    message->roleSize = roleSize;
    message->messageSize = messageSize;

    return (char *)message;
  }
};
