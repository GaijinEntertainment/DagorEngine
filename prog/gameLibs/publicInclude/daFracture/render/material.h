//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <vecmath/dag_vecMathDecl.h>
#include <generic/dag_relocatableFixedVector.h>
#include <shaders/dag_shaderCommon.h>
#include <shaders/dag_shaderMesh.h>


namespace frx
{

struct DestrContext;

struct ShaderMatChannels
{
  struct ChannelDesc
  {
    int offset;
    unsigned size;
    int usage;
    int usageIdx;
    uint32_t type;
    ChannelModifier mod;
  };

  dag::RelocatableFixedVector<ChannelDesc, 3, true> channels;
  int vStride = 0;
  int8_t posChIdx = -1;
  int8_t normChIdx = -1;
  int8_t tcChIdx = -1;

  static ShaderMatChannels create(ShaderMaterial *sh);
};

struct PerInstRenderData
{
  vec4f basePosAndHash = {0.f, 0.f, 0.f, 0.f};
  bool operator==(const PerInstRenderData &rhs) const { return memcmp(this, &rhs, sizeof(PerInstRenderData)) == 0; }
};

struct RenderMatElem
{
  Ptr<ShaderElement> shElem;
  ShaderMesh::Stage stage;
  PerInstRenderData instData;
};

struct RenderMaterial
{
  ShaderMatChannels channels;
  Ptr<ShaderMaterial> shMat;
  RenderMatElem elem;
};

uint16_t add_render_material(DestrContext &ctx, ShaderMaterial *mat, RenderMatElem &&elem, bool allow_reuse = true);

} // namespace frx