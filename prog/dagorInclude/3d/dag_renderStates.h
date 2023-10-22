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

struct RenderStateBits //-V730
{
  uint32_t zwrite : 1, ztest : 1, zFunc : 4; // zFunc:4, stencilFunc:4; can be 3 bits each
  uint32_t stencilRef : 8;
  uint32_t cull : 2;
  uint32_t depthBoundsEnable : 1;
  uint32_t forcedSampleCount : 4;  // Requires caps.hasForcedSamplerCount (DX 11.1)
  uint32_t conservativeRaster : 1; // Requires caps.hasConservativeRassterization (DX 11.3)
  uint32_t zClip : 1;
  uint32_t scissorEnabled : 1;
  uint32_t independentBlendEnabled : 1;
  uint32_t alphaToCoverage : 1;
  uint32_t viewInstanceCount : 2;
  uint32_t colorWr = 0xFFFFFFFF;
  float zBias = 0, slopeZBias = 0;
  StencilState stencil;
};

struct RenderState : public RenderStateBits
{
  // increase if more independent blend parameters are needed
  static constexpr uint32_t NumIndependentBlendParameters = 4;

  struct BlendFactors
  {
    uint8_t src : 4, dst : 4; // 4 bits each
  };
  struct BlendParams
  {
    BlendFactors ablendFactors;
    BlendFactors sepablendFactors;

    uint8_t blendOp : 3, sepablendOp : 3; // can be made 2 bits each, we don't need subtract
    uint8_t ablend : 1, sepablend : 1;
  };

  BlendParams blendParams[NumIndependentBlendParameters];

  // ensure that no padding occured
  static_assert(sizeof(BlendParams) == 3);
  static_assert(sizeof(blendParams) == NumIndependentBlendParameters * sizeof(BlendParams));

  RenderState()
  {
    memset(this, 0, sizeof(RenderState));
    zwrite = 1;
    ztest = 1;
    zFunc = CMPF_GREATEREQUAL;
    stencilRef = 0;
    for (BlendParams &blendParam : blendParams)
    {
      blendParam.ablendFactors.src = BLEND_ONE;
      blendParam.ablendFactors.dst = BLEND_ZERO;
      blendParam.sepablendFactors.src = BLEND_ONE;
      blendParam.sepablendFactors.dst = BLEND_ZERO;
      blendParam.blendOp = BLENDOP_ADD;
      blendParam.sepablendOp = BLENDOP_ADD;
      // done by memset with zero
      // blendParam.ablend = 0;
      // blendParam.sepablend = 0;
    }
    cull = CULL_NONE;
    depthBoundsEnable = 0;
    forcedSampleCount = 0;
    conservativeRaster = 0;
    viewInstanceCount = 0;
    colorWr = 0xFFFFFFFF;
    zClip = 1;
    alphaToCoverage = 0;
    independentBlendEnabled = 0;
  }

  bool operator==(const RenderState &s) const
  {
    if (auto r = memcmp(static_cast<const RenderStateBits *>(this), static_cast<const RenderStateBits *>(&s), //-V1014
          sizeof(RenderStateBits));
        r != 0)
      return false;

    size_t blendParamBytesToCompare = independentBlendEnabled ? sizeof(blendParams) : sizeof(BlendParams);
    return memcmp(blendParams, s.blendParams, blendParamBytesToCompare) == 0;
  }
  bool operator!=(const RenderState &s) const { return !(*this == s); }
};
} // namespace shaders
