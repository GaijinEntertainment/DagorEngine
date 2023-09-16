//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <3d/dag_texMgr.h>
#include <util/dag_oaHashNameMap.h>
#include <util/dag_string.h>
#include <vecmath/dag_vecMathDecl.h>

#include <EASTL/vector.h>

class IesTextureCollection
{
public:
  struct PhotometryData
  {
    float zoom = 1;
    bool rotated = false;
  };

  static IesTextureCollection *getSingleton();
  static IesTextureCollection *acquireRef();
  static void releaseRef();

  explicit IesTextureCollection(eastl::vector<String> &&textures);
  ~IesTextureCollection();

  IesTextureCollection(const IesTextureCollection &) = delete;
  IesTextureCollection &operator=(const IesTextureCollection &) = delete;

  // move semantics implicitly deleted
  // no need for its impelemtation

  int getTextureIdx(const char *name);
  void reloadTextures();
  void addTexture(const char *textureName);
  void close();
  PhotometryData getTextureData(int texIdx) const;

private:
  static IesTextureCollection *instance;
  uint32_t refCount = 0;

  eastl::vector<String> usedTextures;
  eastl::vector<PhotometryData> photometryData;
  TEXTUREID photometryTexId = BAD_TEXTUREID;
  FastNameMap textureNames;
};
