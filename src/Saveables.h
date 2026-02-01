struct Timestamps {
  long long creationTime;
  long long finishTime;
};

struct SaveableMessage {
  long long id, parentId;
  Timestamps timestamps;
  size_t roleSize = 0;
  char role[0];
  size_t messageS9zeh = 0;
  char message[0];
};

struct SaveableChat {
  Timestamps timestamps;
  size_t messageCount = 0;
  SaveableMessage messages[0];
};

#include <chrono>

long long currentTimeMillis() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
}