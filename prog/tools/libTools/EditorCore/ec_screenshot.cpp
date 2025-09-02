// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <debug/dag_log.h>
#include <ioSys/dag_dataBlock.h>

#include "EditorCore/ec_screenshot.h"

namespace
{
void loadGeneralConfig(const DataBlock &blk, ScreenshotConfig &cfg)
{
  const char *format = blk.getStr("format", "JPEG");
  if (strcmp(format, "DDSX") == 0)
  {
    cfg.format = ScreenshotFormat::DDSX;
  }
  else if (strcmp(format, "JPEG") == 0)
  {
    cfg.format = ScreenshotFormat::JPEG;
  }
  else if (strcmp(format, "TGA") == 0)
  {
    cfg.format = ScreenshotFormat::TGA;
  }
  else if (strcmp(format, "PNG") == 0)
  {
    cfg.format = ScreenshotFormat::PNG;
  }
  else if (strcmp(format, "TIFF") == 0)
  {
    cfg.format = ScreenshotFormat::TIFF;
  }
  else
  {
    logerr("Failed to load screenshot format option. Unexpected value %s", format);
    cfg.format = ScreenshotConfig::DEFAULT_FORMAT;
  }

  cfg.jpegQuality = blk.getInt("jpegQ", cfg.jpegQuality);
  cfg.enableTransparentBackground = blk.getBool("enableTransparentBackground", cfg.enableTransparentBackground);
  cfg.enableDebugGeometry = blk.getBool("enableDebugGeometry", cfg.enableDebugGeometry);
  cfg.nameAsCurrentAsset = blk.getBool("nameAsCurrentAsset", cfg.nameAsCurrentAsset);

  if (cfg.jpegQuality <= 0 || cfg.jpegQuality > 100)
  {
    cfg.jpegQuality = ScreenshotConfig::DEFAULT_JPEG_Q;
  }
}


//==================================================================================================
void saveGeneralConfig(DataBlock &blk, const ScreenshotConfig &cfg)
{
  if (cfg.format == ScreenshotFormat::DDSX)
  {
    blk.setStr("format", "DDSX");
  }
  else if (cfg.format == ScreenshotFormat::JPEG)
  {
    blk.setStr("format", "JPEG");
  }
  else if (cfg.format == ScreenshotFormat::TGA)
  {
    blk.setStr("format", "TGA");
  }
  else if (cfg.format == ScreenshotFormat::PNG)
  {
    blk.setStr("format", "PNG");
  }
  else if (cfg.format == ScreenshotFormat::TIFF)
  {
    blk.setStr("format", "TIFF");
  }
  else
  {
    logerr("Unexpected screenshot format while saving options");
  }

  blk.addInt("jpegQ", cfg.jpegQuality);
  blk.addBool("enableTransparentBackground", cfg.enableTransparentBackground);
  blk.addBool("enableDebugGeometry", cfg.enableDebugGeometry);
  blk.addBool("nameAsCurrentAsset", cfg.nameAsCurrentAsset);
}
} // namespace

void saveScreenshotConfig(DataBlock &blk, const ScreenshotConfig &cfg)
{
  if (DataBlock *screenshotBlk = blk.addBlock("screenshot"))
  {
    saveGeneralConfig(*screenshotBlk, cfg);

    screenshotBlk->addInt("width", cfg.width);
    screenshotBlk->addInt("height", cfg.height);
  }
}


//==================================================================================================
void loadScreenshotConfig(const DataBlock &blk, ScreenshotConfig &cfg)
{
  if (const DataBlock *screenshotBlk = blk.getBlockByName("screenshot"))
  {
    loadGeneralConfig(*screenshotBlk, cfg);

    if (cfg.format == ScreenshotFormat::DDSX)
    {
      cfg.format = ScreenshotFormat::JPEG;
    }

    cfg.width = screenshotBlk->getInt("width", cfg.width);
    cfg.height = screenshotBlk->getInt("height", cfg.height);

    if (cfg.width <= 0)
    {
      cfg.width = ScreenshotConfig::DEFAULT_WIDTH;
    }

    if (cfg.height <= 0)
    {
      cfg.height = ScreenshotConfig::DEFAULT_HEIGHT;
    }
  }
}


//==================================================================================================
void saveCubeScreenshotConfig(DataBlock &blk, const ScreenshotConfig &cfg)
{
  if (DataBlock *screenshotBlk = blk.addBlock("cubeScreenshot"))
  {
    saveGeneralConfig(*screenshotBlk, cfg);

    screenshotBlk->addInt("size", cfg.size);
  }
}


//==================================================================================================
void loadCubeScreenshotConfig(const DataBlock &blk, ScreenshotConfig &cfg)
{
  if (const DataBlock *screenshotBlk = blk.getBlockByName("cubeScreenshot"))
  {
    loadGeneralConfig(*screenshotBlk, cfg);

    cfg.size = screenshotBlk->getInt("size", cfg.size);

    const int minSize = 1 << ScreenshotConfig::MIN_CUBE_2_POWER;
    const int maxSize = 1 << ScreenshotConfig::MAX_CUBE_2_POWER;

    if (cfg.size < minSize)
    {
      cfg.size = minSize;
    }
    else if (cfg.size > maxSize)
    {
      cfg.size = maxSize;
    }
    else
    {
      bool equal2pow = false;

      for (int i = (1 << ScreenshotConfig::MIN_CUBE_2_POWER); i <= (1 << ScreenshotConfig::MAX_CUBE_2_POWER); i <<= 1)
        if (cfg.size == i)
        {
          equal2pow = true;
          break;
        }

      if (!equal2pow)
        cfg.size = minSize;
    }
  }
}


//==================================================================================================
const char *getFileExtFromFormat(ScreenshotFormat fmt)
{
  switch (fmt)
  {
    case ScreenshotFormat::JPEG: return ".jpg";
    case ScreenshotFormat::TGA: return ".tga";
    case ScreenshotFormat::PNG: return ".png";
    case ScreenshotFormat::TIFF: return ".tiff";
    case ScreenshotFormat::DDSX: return ".ddsx";
    default:
    {
      logerr("Unexpected screenshot format while getting extension");
      return ".jpg";
    }
  }
}