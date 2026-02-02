#pragma once

#include <chrono>
#include <random>

long long randomId() {
    thread_local std::mt19937_64 gen(std::random_device{}() ^ std::chrono::steady_clock::now().time_since_epoch().count());
    return gen();
}

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

  void update() {
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