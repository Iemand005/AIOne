#pragma once

#include "Random.h"

struct Timestamps {
  long long creationTime = 0, modificationTime = 0;

  Timestamps() = default;
  
  Timestamps(const Timestamps& other) = default;
  
  Timestamps(Timestamps&& other) noexcept = default;
  
  Timestamps& operator=(const Timestamps& other) = default;
  
  Timestamps& operator=(Timestamps&& other) noexcept = default;

  long long currentTimeMillis() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
  }

  long long randomId() {
    return Random::int64();
  }

  void start() {
    creationTime = currentTimeMillis();
  } 

  void update() {
    modificationTime = currentTimeMillis();
  }
};
