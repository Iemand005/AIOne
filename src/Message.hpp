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

  SaveableMessage toSaveable() {
    size_t messageSize = content.size();
    size_t messageSize = role.size();
    size_t size = sizeof(SaveableMessage) + messageSize + messageSize
    SaveableMessage message;
    message.id = this->id;
    message.parentId = this->parentId;
    message.messageLength = content.length();
    message.message = ;

    return {

    }
  }
};
