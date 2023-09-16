//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <math/dag_Point2.h>
#include <math/dag_Point4.h>
#include <dag/dag_vector.h>
#include <3d/dag_resPtr.h>

struct TextureInfo;

namespace atlas_tex_manager
{
struct AtlasData;

struct TexRec
{
  dag::Vector<UniqueTex> atlasTex;
  dag::Vector<unsigned> atlasTexFmt;
  AtlasData *ad = nullptr;
  Point2 size = Point2(0, 0);

  bool initAtlas(int tex_count, dag::ConstSpan<unsigned> tex_fmt, const char *tex_name, IPoint2 tex_size, int mip_count);
  void resetAtlas();
  bool isInited() const { return ad != nullptr; }
  int addTextureToAtlas(TextureInfo ti, Point4 &texcoords, bool allow_discard = true);
  bool writeTextureToAtlas(Texture *tex, int atlas_index, int layer);
  int mipCount = 0;
  bool compressed = false;
  int atlasTexcount = 0;
  Point4 getTc(int index);
};
}; // namespace atlas_tex_manager
