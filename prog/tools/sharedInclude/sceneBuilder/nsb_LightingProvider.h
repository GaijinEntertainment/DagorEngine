// Copyright 2023 by Gaijin Games KFT, All rights reserved.
#ifndef __GAIJIN_NSB_LIGHTINGPROVIDER_H__
#define __GAIJIN_NSB_LIGHTINGPROVIDER_H__
#pragma once


#include <sceneBuilder/nsb_decl.h>
#include <math/dag_color.h>
#include <math/dag_e3dColor.h>
#include <math/dag_Point3.h>
#include <math/integer/dag_IPoint2.h>

struct TexImage32;

namespace StaticSceneBuilder
{
class LightingProvider
{
public:
  enum
  {
    LOAD_OK,
    LOAD_NOFILE,
    LOAD_WRONGSIGNATURE,
    LOAD_WRONGSIZE,
    LOAD_UUIDMISMATCH,
    LOAD_FAILED
  };

  Color3 getSunColor() const { return Color3(1, 1, 1); }
  Point3 getSunDirection() const { return Point3(0, 1, 0); }

  int getLightmapCount() const { return 0; }
  TexImage32 *getLightmapImage(int /*lmid*/) const { return nullptr; }
  TexImage32 *getLightmapImageSunBump(int /*lmid*/) const { return nullptr; }
  IPoint2 getLightmapCropShr(int /*lmid*/) const { return IPoint2(0, 0); }

  E3DCOLOR getSunBump(int /*index*/) const { return 0; }
  E3DCOLOR getVltmapColor(int /*index*/) const { return 0; }
};
}; // namespace StaticSceneBuilder


#endif
