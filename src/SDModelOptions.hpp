#pragma once

struct SDModelOptions {
  bool vaeDecodeOnly = true;
  bool freeParamsImmediately = false;
  short threadCount = 6;

  bool keepClipOnCpu = false;
  bool keepControlNetOnCpu = false;
  bool keepVaeOnCpu = false;

  bool flashAttention = true;
};