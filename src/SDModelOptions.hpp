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

  std::string vaePath = "";
  std::string taePath = "";

  std::string clipGPath = "";
  std::string clipLPath = "";

  ProgressCallback onProgress = nullptr;
};
