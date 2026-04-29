// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "dynamicMirrorNodes.h"

static const auto gbuffer_view_sizeVarId = ShaderVariableInfo("gbuffer_view_size");
static const auto screen_pos_to_texcoordVarId = ShaderVariableInfo("screen_pos_to_texcoord");
static const auto view_vecLTVarId = ShaderVariableInfo("view_vecLT");
static const auto view_vecRTVarId = ShaderVariableInfo("view_vecRT");
static const auto view_vecLBVarId = ShaderVariableInfo("view_vecLB");
static const auto view_vecRBVarId = ShaderVariableInfo("view_vecRB");

DynamicMirrorShaderVars::ScopedVars::ScopedVars(
  const IPoint4 &gbuffer_view_size, const Color4 &screen_pos_to_texcoord, const ViewVecs &view_vecs)
{
  originalGbufferViewSize = ShaderGlobal::get_int4(gbuffer_view_sizeVarId);
  originalScreenPosToTexcoord = ShaderGlobal::get_float4(screen_pos_to_texcoordVarId);
  originalViewVecs.viewVecLT = Point4::rgba(ShaderGlobal::get_float4(view_vecLTVarId));
  originalViewVecs.viewVecRT = Point4::rgba(ShaderGlobal::get_float4(view_vecRTVarId));
  originalViewVecs.viewVecLB = Point4::rgba(ShaderGlobal::get_float4(view_vecLBVarId));
  originalViewVecs.viewVecRB = Point4::rgba(ShaderGlobal::get_float4(view_vecRBVarId));

  ShaderGlobal::set_int4(gbuffer_view_sizeVarId, gbuffer_view_size);
  ShaderGlobal::set_float4(screen_pos_to_texcoordVarId, screen_pos_to_texcoord);
  ShaderGlobal::set_float4(view_vecLTVarId, view_vecs.viewVecLT);
  ShaderGlobal::set_float4(view_vecRTVarId, view_vecs.viewVecRT);
  ShaderGlobal::set_float4(view_vecLBVarId, view_vecs.viewVecLB);
  ShaderGlobal::set_float4(view_vecRBVarId, view_vecs.viewVecRB);
}
DynamicMirrorShaderVars::ScopedVars::~ScopedVars()
{
  ShaderGlobal::set_int4(gbuffer_view_sizeVarId, originalGbufferViewSize);
  ShaderGlobal::set_float4(screen_pos_to_texcoordVarId, originalScreenPosToTexcoord);
  ShaderGlobal::set_float4(view_vecLTVarId, originalViewVecs.viewVecLT);
  ShaderGlobal::set_float4(view_vecRTVarId, originalViewVecs.viewVecRT);
  ShaderGlobal::set_float4(view_vecLBVarId, originalViewVecs.viewVecLB);
  ShaderGlobal::set_float4(view_vecRBVarId, originalViewVecs.viewVecRB);
}