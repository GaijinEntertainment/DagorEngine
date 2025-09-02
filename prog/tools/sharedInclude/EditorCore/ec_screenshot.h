// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

class DataBlock;

enum class ScreenshotFormat
{
  JPEG,
  TGA,
  PNG,
  TIFF,
  DDSX,
};

enum class ScreenshotDlgMode
{
  DA_EDITOR,
  ASSET_VIEWER,
};

struct ScreenshotConfig
{
  inline static constexpr ScreenshotFormat DEFAULT_FORMAT = ScreenshotFormat::JPEG;
  inline static constexpr int DEFAULT_WIDTH = 1024;
  inline static constexpr int DEFAULT_HEIGHT = 768;
  inline static constexpr int DEFAULT_SIZE = 512;
  inline static constexpr int DEFAULT_JPEG_Q = 80;
  inline static constexpr int MIN_CUBE_2_POWER = 5;
  inline static constexpr int MAX_CUBE_2_POWER = 11;

  static ScreenshotConfig getDefaultCfg() { return ScreenshotConfig{.width = DEFAULT_WIDTH, .height = DEFAULT_HEIGHT}; }
  static ScreenshotConfig getDefaultCubeCfg() { return ScreenshotConfig{.size = DEFAULT_SIZE}; }

  union
  {
    struct
    {
      int width;
      int height;
    };
    int size;
  };

  ScreenshotFormat format = DEFAULT_FORMAT;
  int jpegQuality = DEFAULT_JPEG_Q;
  bool enableTransparentBackground = false;
  bool enableDebugGeometry = false;
  bool nameAsCurrentAsset = false;
};

void saveScreenshotConfig(DataBlock &blk, const ScreenshotConfig &cfg);
void loadScreenshotConfig(const DataBlock &blk, ScreenshotConfig &cfg);

void saveCubeScreenshotConfig(DataBlock &blk, const ScreenshotConfig &cfg);
void loadCubeScreenshotConfig(const DataBlock &blk, ScreenshotConfig &cfg);

const char *getFileExtFromFormat(ScreenshotFormat fmt);
