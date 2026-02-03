#pragma once

struct VAETilingOptions {
  bool enabled = false;
  int tileWidth = 128, tileHeight = 128;
  float overlap = 0.5f;
};

struct SDImageOptions {
  int width = 512, height = 512;
  int stepCount = 25;
  float cfgScale = 6.0f;
  int clipSkip = -1;
  VAETilingOptions tiling;
};