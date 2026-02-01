struct Timestamps {
  long long creationTime;
  long long finishTime;
};

struct SaveableMessageHeader {
  long long id, parentId;
  Timestamps timestamps;
  size_t roleSize = 0;
  size_t messageSize = 0;
};

struct SaveableChatHeader {
  Timestamps timestamps;
  size_t messageCount = 0;
};

#include <chrono>

long long currentTimeMillis() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
}

template<typename T>
T *allocateStruct(size_t extraSize) {
  return (T *)malloc(sizeof(T) + extraSize);
};