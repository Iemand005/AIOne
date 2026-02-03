#pragma once

#include <chrono>
#include <random>

struct Random {
  static int64_t int64() {
    thread_local std::mt19937_64 gen(std::random_device{}() ^ 
        std::chrono::steady_clock::now().time_since_epoch().count());
    return gen();
  }
};