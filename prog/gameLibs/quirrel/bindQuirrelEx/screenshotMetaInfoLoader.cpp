// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "screenshotMetaInfoLoader.h"
#include <image/dag_jpeg.h>
#include <image/dag_png.h>
#include <image/dag_texPixel.h>
#include <ioSys/dag_fileIo.h>

namespace bindquirrel
{

DataBlock get_meta_info_from_screenshot(const char *screenshot_path)
{
  FullFileLoadCB crd(screenshot_path);
  if (!crd.fileHandle)
    return DataBlock();

  eastl::string comment;
  TexImage32 *img = load_jpeg32(crd, stdmem_ptr(), &comment);

  if (!img)
  {
    crd.seekto(0);
    bool unused = false;
    img = load_png32(crd, stdmem_ptr(), &unused, &comment);
  }

  if (!img)
    return DataBlock();
  delete img;

  if (comment.empty())
    return DataBlock();

  DataBlock metaInfo;
  if (!metaInfo.loadText(comment.c_str(), comment.size(), screenshot_path))
    return DataBlock();

  return metaInfo;
}

} // namespace bindquirrel