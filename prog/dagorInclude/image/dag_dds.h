//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <3d/dag_tex3d.h>

// forward declarations for external classes
class IGenLoad;

struct ImageInfoDDS
{
  bool cube_map;
  bool volume_tex;
  unsigned format;
  unsigned width;
  unsigned height;
  unsigned shift;
  unsigned nlevels;
  bool dxt;
  unsigned pitch;
  void *pixels[16];
};

bool load_dds(void *ptr, int len, int levels, int topmipmap, ImageInfoDDS &image_info);
void *load_dds_file(const char *fn, int levels, int quality, IMemAlloc *mem, ImageInfoDDS &image_info, bool ignore_missing = false);
Texture *create_dds_texture(bool srgb, const ImageInfoDDS &image_info, const char *tex_name = NULL, int flags = 0);
