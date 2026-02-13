#pragma once

#include <functional>
#include <memory>

typedef std::function<void(const float progress)> ProgressCallback;
typedef std::shared_ptr<ProgressCallback> ProgressCallbackPtr;

template<typename T>
using FinishedTCallback = std::function<void(T)>;

typedef FinishedTCallback<void> FinishedCallback;

// template <typename BaseT>
// struct ResponseAsync : BaseT
// {
//     FinishedCallback onDone = nullptr;

//     bool done() {
//       if (onDone) {
//         onDone();
//         return true;
//       }
//       return false;
//     }
// };

struct CallbackAsyncBase
{
    FinishedCallback onDone = nullptr;

    bool done() const {
      if (onDone) {
        onDone();
        return true;
      }
      return false;
    }
};

