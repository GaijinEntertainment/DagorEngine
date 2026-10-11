// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <drv/3d/dag_tex3d.h>
#include <drv/3d/dag_samplerHandle.h>
#include <math/dag_Point3.h>
#include <math/dag_TMatrix.h>
#include <math/dag_TMatrix4.h>
#include <math/integer/dag_IPoint2.h>
#include <render/daFrameGraph/daFG.h>
#include <dag/dag_vector.h>
#include <EASTL/fixed_function.h>

namespace bvh
{
struct Context;
using ContextId = Context *;
} // namespace bvh

namespace rtdl
{
inline constexpr const char *output_texture_name = "rt_direct_lighting_output";
inline constexpr const char *sun_shadow_texture_name = "rt_direct_lighting_sun_shadow";
inline constexpr const char *frame_params_blob_name = "rt_direct_lighting_params";
inline constexpr const char *rendered_blob_name = "rt_direct_lighting_rendered";

struct FrameParams
{
  bvh::ContextId ctxId = nullptr;
  Texture *csmTexture = nullptr;
  d3d::SamplerHandle csmSampler = d3d::INVALID_SAMPLER_HANDLE;
  Texture *vsmTexture = nullptr;
  d3d::SamplerHandle vsmSampler = d3d::INVALID_SAMPLER_HANDLE;
  TMatrix4 projTm = TMatrix4::IDENT;
  TMatrix viewTm = TMatrix::IDENT;
  Point3 viewPos = Point3::ZERO;
  IPoint2 dynamicResolution = IPoint2::ZERO;
  float znear = 0.1f;
  float zfar = 1000.f;
  float defaultLightSourceRadius = 0.1f;
  int sunQuality = 1;
  bool hasNuke = false;
  bool resetHistory = false;
  bool render = false;
};

struct NodeParams
{
  const char *resolutionName = "main_view";
  IPoint2 resolution = IPoint2::ZERO;
  eastl::fixed_function<32, void(dafg::Registry)> declareInputs;
  eastl::fixed_function<32, void()> setShaderState;
  bool dynamicLights = true;
};

void initialize();
void teardown();
bool is_initialized();

void register_nodes(dafg::NameSpace ns, const NodeParams &params, dag::Vector<dafg::NodeHandle> &nodes);
} // namespace rtdl
