// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <drv/3d/dag_tex3d.h>
#include <drv/3d/dag_samplerHandle.h>
#include <math/dag_Point3.h>
#include <math/dag_TMatrix4.h>
#include <dag/dag_vector.h>
#include <EASTL/fixed_function.h>
#include <render/daFrameGraph/daFG.h>
#include <render/denoiser/denoiser_api.h>

namespace bvh
{
struct Context;
using ContextId = Context *;
} // namespace bvh

namespace rtsm
{

// The per frame data of the sun shadow trace: a game node creates the blob with create_frame_params and fills it. That
// node is one of the root namespace, since the lib nodes register at the root and read the blob there.
struct FrameParams
{
  bvh::ContextId ctxId = nullptr;
  Point3 viewPos = Point3::ZERO;
  Point3 lightDir = Point3(0, -1, 0);
  TMatrix4 projTm = TMatrix4::IDENT;
  eastl::optional<denoiser::TraceView> lensView; // the scope lens camera while the lens is rendered
  Texture *csmTexture = nullptr;
  d3d::SamplerHandle csmSampler = d3d::INVALID_SAMPLER_HANDLE;
  Texture *vsmTexture = nullptr;
  d3d::SamplerHandle vsmSampler = d3d::INVALID_SAMPLER_HANDLE;
  int quality = 0; // 0: a ray per pixel, 1 and 2: a ray per two pixels with the high or the low fill
  bool hasNuke = false;
  bool render = false;
};

struct NodeParams
{
  const char *motionVectorsName = nullptr; // the full resolution motion vectors of the denoiser
  denoiser::DeclareInputs declareInputs;   // the gbuffer, the depth and the ordering of the trace
  // sets the view globals the trace shaders read, called before the trace: the view vectors, zn_zfar and world_view_pos
  eastl::fixed_function<32, void()> setShaderState;
  eastl::fixed_function<32, void()> resetShaderState; // undoes it after the trace
  denoiser::ViewStateHooks viewState;                 // the hooks of a frame with a lens view
};

void initialize();
void teardown();

denoiser::ParamsHandle<FrameParams> create_frame_params(dafg::Registry registry);
// The trace node and the denoise node of the sun shadow, see denoiser::make_shadow_node
void register_nodes(const NodeParams &params, dag::Vector<dafg::NodeHandle> &nodes);

void render_dynamic_light_shadows(bvh::ContextId context_id, const Point3 &view_pos, Texture *dynamic_lighting_texture,
  float light_radius, bool soft_shadow, bool has_nuke = false);

} // namespace rtsm
