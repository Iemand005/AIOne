#pragma once

#include <chrono>
#include <random>

long long randomId() {
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<long long> dist(0, LLONG_MAX);
    return dist(gen);
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