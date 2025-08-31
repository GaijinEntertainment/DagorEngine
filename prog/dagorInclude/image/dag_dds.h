//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <drv/3d/dag_tex3d.h>

// forward declarations for external classes
class IGenLoad;

struct ImageInfoDDS
{
  unsigned format, d3dFormat;
  unsigned width, height, depth;
  unsigned short nlevels, shift;
  bool cube_map, volume_tex, dxt;
  unsigned pitch;
  void *pixels[16];
};

bool load_dds(void *ptr, int len, int levels, int topmipmap, ImageInfoDDS &image_info);
void *load_dds_file(const char *fn, int levels, int quality, IMemAlloc *mem, ImageInfoDDS &image_info, bool ignore_missing = false);
Texture *create_dds_texture(bool srgb, const ImageInfoDDS &image_info, const char *tex_name = NULL, int flags = 0);

bool read_dds_info(const char *fn, ImageInfoDDS &out_image_info);
