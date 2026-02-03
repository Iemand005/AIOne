#pragma once

struct VAETilingOptions {
  bool enabled = false;
  int tileWidth = 128, tileHeight = 128;
  float overlap = 0.5f;
};

struct SDImageOptions {
  int width = 512, height = 512;
  int batchSize = 1;
  int stepCount = 25, clipSkip = -1;
  int64_t seed = 42;
  // bool randomSeed = true;
  float cfgScale = 6.0f;
  VAETilingOptions tiling;
};