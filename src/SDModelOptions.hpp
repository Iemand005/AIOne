#pragma once

#include <string>

#include "ModelOptions.h"

struct SDModelOptions : ModelOptions {
  bool vaeDecodeOnly = true;
  bool freeParamsImmediately = false;

  bool keepClipOnCpu = false;
  bool keepControlNetOnCpu = false;
  bool keepVaeOnCpu = false;

  bool useMmap = false;

  std::string vaePath = "";
};
