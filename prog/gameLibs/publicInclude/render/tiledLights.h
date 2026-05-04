//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <3d/dag_resPtr.h>
#include <3d/dag_resizableTex.h>
#include <EASTL/unique_ptr.h>
#include <math/integer/dag_IPoint2.h>
#include <shaders/dag_computeShaders.h>
#include <shaders/dag_overrideStateId.h>
#include <vecmath/dag_vecMathDecl.h>

class TiledLights
{
  ResizableTex tilesRT;
  UniqueBufHolder lightsListBuf;
  UniqueBufHolder zbinningLUT;
  UniqueBufHolder zBinBeginsEndsBuf;
  float maxLightsDist = 500;
  Tab<uint32_t> zBinningData;
  Tab<uint16_t> zBinBegins, zBinEnds;
  IPoint2 tilesGridSize;
  uint32_t omniLightCount, spotLightCount;
  bool useTilesRT = false;

  int maxDistanceVarId = -1;
  int viewDirVarId = -1;
  int viewPosVarId = -1;
  Color4 binningViewDir;
  Color4 binningViewPos;

  ShaderMaterial *spotLightsMat = 0;
  ShaderElement *spotLightsElem = 0;
  ShaderMaterial *omniLightsMat = 0;
  ShaderElement *omniLightsElem = 0;
  ComputeShader zBinningOmniElem;
  ComputeShader zBinningSpotElem;
  ComputeShader zBinningClearElem;
  ComputeShader zBinningPackElem;
  shaders::UniqueOverrideStateId conservativeOverrideId;

  Sbuffer *spotsIndexBuf = NULL;

  uint32_t spotsSlices = 8;
  uint32_t spotsStacks = 10;
  uint32_t spotsVertexCount = 0;
  uint32_t spotsFaceCount = 0;
  bool spotFilled = false;

  void prepareZBinningLUT(const Tab<vec4f> &omni_ligth_bounds, const Tab<vec4f> &spot_light_bounds, const vec4f &cur_view_pos,
    const vec4f &cur_view_dir);
  void fillzBins(uint32_t max_id, uint32_t offset, const Tab<vec4f> &ligth_bounds, const vec4f &cur_view_pos,
    const vec4f &cur_view_dir);
  void createSpotLightIb();
  bool isGPUZBinning() const;

public:
  void fillSpotLightIb();
  TiledLights(float max_lights_dist);
  ~TiledLights();
  void resizeTilesGrid(uint32_t width, uint32_t height);
  void setResolution(uint32_t width, uint32_t height);
  void changeResolution(uint32_t width, uint32_t height);
  void prepare(const Tab<vec4f> &omni_ligth_bounds, const Tab<vec4f> &spot_light_bounds, const vec4f &cur_view_pos,
    const vec4f &cur_view_dir);
  void applyBinning();
  void computeTiledLigths(const bool clear_lights = true);
  void setMaxLightsDist(const float max_lights_dist);
  void afterResetDevice();
  void beforeResetDevice();
};
