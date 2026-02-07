#pragma once

#include <functional>
#include <memory>

typedef std::function<void(const float progress)> ProgressCallback;
typedef std::shared_ptr<ProgressCallback> ProgressCallbackPtr;

template<typename T>
using FinishedTCallback = std::function<void(T)>;

typedef FinishedTCallback<void> FinishedCallback;