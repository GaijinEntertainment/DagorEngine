//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <string.h>

#include <3d/dag_3dConst_base.h>
#include <shaders/dag_overrideStates.h>

namespace shaders
{
struct RenderState
{
  uint32_t zwrite : 1, ztest : 1, zFunc : 4; // zFunc:4, stencilFunc:4; can be 3 bits each
  uint32_t stencilRef : 8;
  uint32_t ablendSrc : 4, ablendDst : 4;       // 4 bits each
  uint32_t sepablendSrc : 4, sepablendDst : 4; // 4 bits each
  uint32_t blendOp : 3, sepablendOp : 3;       // can be made 2 bits each, we don't need subtract
  uint32_t cull : 2;
  uint32_t ablend : 1, sepablend : 1;
  uint32_t depthBoundsEnable : 1;
  uint32_t forcedSampleCount : 4;  // Requires caps.hasForcedSamplerCount (DX 11.1)
  uint32_t conservativeRaster : 1; // Requires caps.hasConservativeRassterization (DX 11.3)
  uint32_t zClip : 1;
  uint32_t scissorEnabled : 1;
  uint32_t alphaToCoverage : 1;
  uint32_t viewInstanceCount : 2;
  uint32_t colorWr = 0xFFFFFFFF;
  float zBias = 0, slopeZBias = 0;
  StencilState stencil;
  RenderState()
  {
    memset(this, 0, sizeof(RenderState));
    zwrite = 1;
    ztest = 1;
    zFunc = CMPF_GREATEREQUAL;
    stencilRef = 0;
    ablendSrc = BLEND_ONE;
    ablendDst = BLEND_ZERO;
    sepablendSrc = BLEND_ONE;
    sepablendDst = BLEND_ZERO;
    blendOp = BLENDOP_ADD;
    sepablendOp = BLENDOP_ADD;
    cull = CULL_NONE;
    ablend = 0;
    sepablend = 0;
    depthBoundsEnable = 0;
    forcedSampleCount = 0;
    conservativeRaster = 0;
    viewInstanceCount = 0;
    colorWr = 0xFFFFFFFF;
    zClip = 1;
    alphaToCoverage = 0;
  }

  bool operator==(const RenderState &s) const { return memcmp(this, &s, sizeof(RenderState)) == 0; } //-V1014
  bool operator!=(const RenderState &s) const { return !(*this == s); }
};
} // namespace shaders
