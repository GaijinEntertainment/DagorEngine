// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "screenshotMetaInfoLoader.h"
#include <image/dag_loadImage.h>
#include <image/dag_jpeg.h>
#include <image/dag_png.h>
#include <image/dag_texPixel.h>
#include <ioSys/dag_fileIo.h>
#include <util/dag_string.h>

namespace bindquirrel
{

DataBlock get_meta_info_from_screenshot(const char *screenshot_path)
{
  DataBlock metaInfo;
  String errorMessage;
  load_meta_info_from_image(screenshot_path, metaInfo, errorMessage);
  return metaInfo;
}

} // namespace bindquirrel