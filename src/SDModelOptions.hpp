#pragma once

#include <string>

#include "ModelOptions.h"
#include "Callbacks.h"

struct SDModelOptions : ModelOptions {
  bool vaeDecodeOnly = true;
  bool freeParamsImmediately = false;

  bool keepClipOnCpu = false;
  bool keepControlNetOnCpu = false;
  bool keepVaeOnCpu = false;

  bool useMmap = false;
  std::string backend = "";

  std::string vaePath = "";
  std::string taePath = "";

  std::string clipGPath = "";
  std::string clipLPath = "";
  std::string t5xxlPath = "";

  ProgressCallback onProgress = nullptr;
};

struct SDModelOptionsAsync : CallbackAsyncBase, SDModelOptions
{
    // FinishedCallback onDone = nullptr;

    // bool done() {
    //   if (onDone) {
    //     onDone();
    //     return true;
    //   }
    //   return false;
    // }
};
