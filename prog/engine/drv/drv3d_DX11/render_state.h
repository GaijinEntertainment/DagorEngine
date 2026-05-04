// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "driver_defs.h"
#include <drv/3d/dag_texture.h>
#include "frameStateTM.inc.h"
#include "buffers.h"
#include "shaders.h"
#include "states.h"
#include "validation.h" // for ENABLE_GENERIC_RENDER_PASS_VALIDATION

namespace drv3d_dx11
{

// per thread state
struct RenderState : public FrameStateTM
{
  bool modified;
  bool rtModified;
  uint8_t viewModified; // VIEWMOD_
  bool scissorModified;
  bool vsModified;
  bool psModified;
  bool csModified;
  bool vertexInputModified;
  bool rasterizerModified;
  bool alphaBlendModified;
  bool depthStencilModified;
#if ENABLE_GENERIC_RENDER_PASS_VALIDATION
  bool isGenericRenderPassActive = false;
#endif

  bool dualSourceBlendActive;
  bool srgb_bb_write;
  uint8_t hdgBits;      // flag bits HS(0x1), DS(0x2), GS(0x4) for current compound VS prog (and dirty(0x80) for faster cmp)
  uint8_t hullTopology; // 5 bits is enough but it will start from 33
  int8_t shaderMaxRtv;  // RTV max index of those declared in the bound shader for UAV conflict avoidance

  //- input assembler ------------------------------------

  VertexInput nextVertexInput;
  VertexInput currVertexInput;
  uint8_t currPrimType; // PRIM_UNDEF == 0 - unitialized

  //- shaders --------------------------------------------
  VDECL nextVdecl;
  int nextVertexShader;
  int nextPixelShader;
  int nextComputeShader;

  ConstantBuffers constants;
  TextureFetchState texFetchState;

  //- rasterizer -----------------------------------------

  RasterizerState nextRasterizerState;

  //- output merger --------------------------------------
  Driver3dRenderTarget nextRtState;
  Driver3dRenderTarget currRtState;

  uint8_t stencilRef;
  float blendFactor[BLEND_FACTORS_COUNT];

  int8_t maxUsedTarget;
  uint16_t currentFrameDip;

  RenderState() :
    modified(true),
    rtModified(true),
    viewModified(VIEWMOD_FULL),
    scissorModified(false),
    vsModified(true),
    psModified(true),
    csModified(true),
    vertexInputModified(true),
    rasterizerModified(true),
    alphaBlendModified(true),
    depthStencilModified(true),
    dualSourceBlendActive(false),
    srgb_bb_write(0),
    hdgBits(0),
    hullTopology(0),
    shaderMaxRtv(-1),
    nextVertexInput{},
    currVertexInput{},
    currPrimType(0),
    nextVdecl(drv3d_generic::BAD_HANDLE),
    nextVertexShader(drv3d_generic::BAD_HANDLE),
    nextPixelShader(drv3d_generic::BAD_HANDLE),
    nextComputeShader(drv3d_generic::BAD_HANDLE),
    nextRasterizerState{},
    nextRtState{},
    currRtState{},
    stencilRef(0),
    maxUsedTarget(-1),
    currentFrameDip(0)
  {
    blendFactor[0] = 1.0f;
    blendFactor[1] = 1.0f;
    blendFactor[2] = 1.0f;
    blendFactor[3] = 1.0f;
  }
  void reset()
  {
    // this is a bit hacky. We'd better correctly reset everything
    ConstantBuffers savedConstants = constants;
    int dip = currentFrameDip;
    *this = RenderState();
    currentFrameDip = dip; //-V1048
    constants = savedConstants;
    nextRtState.setBackbufColor();
  }

  void resetFrame()
  {
    modified = true;
    srgb_bb_write = false;
    rtModified = true;
    viewModified = VIEWMOD_FULL;
    scissorModified = true;
    psModified = true;
    vsModified = true;
    hdgBits = 0x80;
    vertexInputModified = true;
    rasterizerModified = true;
    alphaBlendModified = true;
    depthStencilModified = true;
    for (auto &resources : texFetchState.resources)
      resources.modifiedMask = resources.samplersModifiedMask = 0xffffffff;
    currentFrameDip = 0;
    resetDipCounter();
  };

  void resetCached() {}

  void resetDipCounter() { currentFrameDip = 0; }
  ~RenderState()
  {
    for (int i = 0; i < MAX_PS_SAMPLERS; ++i)
      d3d::set_tex(STAGE_PS, i, nullptr);
    for (int i = 0; i < MAX_VS_SAMPLERS; ++i)
      d3d::set_tex(STAGE_VS, i, nullptr);
    for (int i = MAX_PS_SAMPLERS; i < MAX_RESOURCES; ++i)
      d3d::set_buffer(STAGE_PS, i, 0);
    for (int i = MAX_VS_SAMPLERS; i < MAX_RESOURCES; ++i)
      d3d::set_buffer(STAGE_VS, i, 0);
  }
};

} // namespace drv3d_dx11
