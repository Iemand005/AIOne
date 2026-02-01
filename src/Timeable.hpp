#pragma once

#include <chrono>

struct Timestamps {
  long long creationTime, modificationTime;

  long long currentTimeMillis() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
  }

  void start() {
    creationTime = currentTimeMillis();
  } 

  void finish() {
    modificationTime = currentTimeMillis();
  }
};

// class Timeable {

// public:
//   long long currentTimeMillis() {
//     return std::chrono::duration_cast<std::chrono::milliseconds>(
//         std::chrono::system_clock::now().time_since_epoch()
//     ).count();
//   }

//   Timestamps newTimeStamps() {

//   }
// };