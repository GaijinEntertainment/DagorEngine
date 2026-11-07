// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

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

namespace rtao
{

// The per frame data of the trace: a game node creates the blob with create_frame_params and fills it. That
// node is one of the root namespace, since the lib nodes register at the root and read the blob there.
struct FrameParams
{
  bvh::ContextId ctxId = nullptr;
  TMatrix4 projTm = TMatrix4::IDENT;
  eastl::optional<denoiser::TraceView> lensView; // the scope lens camera while the lens is rendered
  bool performanceMode = true;                   // the denoiser quality
  bool checkerboard = true;                      // a ray per two pixels, never with ray reconstruction
  bool render = false;
};

struct NodeParams
{
  const char *motionVectorsName = nullptr;     // the motion vectors of the denoiser, a full res AO reads it
  const char *halfMotionVectorsName = nullptr; // a half res AO reads it
  denoiser::DeclareInputs declareInputs;       // the gbuffer, the depth and the ordering of the trace
  // sets the view globals the trace shaders read, called before the trace: the view vectors, zn_zfar and world_view_pos
  eastl::fixed_function<32, void()> setShaderState;
  eastl::fixed_function<32, void()> resetShaderState; // undoes it after the trace
  denoiser::ViewStateHooks viewState;                 // the hooks of a frame with a lens view
};

void initialize(bool half_res);
void teardown();

denoiser::ParamsHandle<FrameParams> create_frame_params(dafg::Registry registry);
// The trace node and the denoise node, see denoiser::make_ao_node
void register_nodes(const NodeParams &params, dag::Vector<dafg::NodeHandle> &nodes);

} // namespace rtao
