#pragma once

struct VAETilingOptions {
  bool enabled = false;
  int tileWidth = 128, tileHeight = 128;
  float overlap = 0.5f;
};

enum SDPreviewMode {
    None,
    VAE,
    TAE,
    Proj
};

struct SDImageOptions {
  int width = 512, height = 512;
  int batchSize = 1;
  int stepCount = 25, clipSkip = -1;
  long long seed = 42;
  // bool randomSeed = true;
  float cfgScale = 6.0f;

  SDPreviewMode previewMode = SDPreviewMode::Proj;
  VAETilingOptions tiling;
};
