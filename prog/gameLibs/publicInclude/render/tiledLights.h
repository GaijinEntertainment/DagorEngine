//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <3d/dag_resPtr.h>
#include <3d/dag_textureIDHolder.h>
#include <EASTL/unique_ptr.h>
#include <math/integer/dag_IPoint2.h>
#include <shaders/dag_overrideStateId.h>
#include <vecmath/dag_vecMathDecl.h>

class TiledLights
{
  ResizableTextureIDHolder tilesRT;
  UniqueBufHolder lightsListBuf;
  UniqueBufHolder zbinningLUT;
  float maxLightsDist = 500;
  Tab<uint32_t> zBinningData;
  Tab<uint16_t> zBinBegins, zBinEnds;
  IPoint2 tilesGridSize;
  uint32_t omniLightCount, spotLightCount;

  ShaderMaterial *spotLightsMat = 0;
  ShaderElement *spotLightsElem = 0;
  ShaderMaterial *omniLightsMat = 0;
  ShaderElement *omniLightsElem = 0;
  shaders::UniqueOverrideStateId conservativeOverrideId;

  void prepareZBinningLUT(const Tab<vec4f> &omni_ligth_bounds, const Tab<vec4f> &spot_light_bounds, const vec4f &cur_view_pos,
    const vec4f &cur_view_dir);
  void fillzBins(uint32_t max_id, uint32_t offset, const Tab<vec4f> &ligth_bounds, const vec4f &cur_view_pos,
    const vec4f &cur_view_dir);

public:
  TiledLights(float max_lights_dist);
  ~TiledLights();
  void setResolution(uint32_t width, uint32_t height);
  void changeResolution(uint32_t width, uint32_t height);
  void prepare(const Tab<vec4f> &omni_ligth_bounds, const Tab<vec4f> &spot_light_bounds, const vec4f &cur_view_pos,
    const vec4f &cur_view_dir);
  void fillBuffers();
  void computeTiledLigths();
  void setMaxLightsDist(const float max_lights_dist);
};
