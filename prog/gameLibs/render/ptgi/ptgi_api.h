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

namespace ptgi
{

enum class Quality
{
  Low,
  Medium,
  High,
};

// The per frame data of the trace: a game node creates the blob with create_frame_params and fills it. That
// node is one of the root namespace, since the lib nodes register at the root and read the blob there.
struct FrameParams
{
  bvh::ContextId ctxId = nullptr;
  TMatrix4 projTm = TMatrix4::IDENT;
  Quality quality = Quality::Low;
  bool inCockpit = false;
  bool halfRate = false; // a ray per two pixels with ray reconstruction
  bool render = false;
};

struct NodeParams
{
  const char *motionVectorsName = nullptr;     // the motion vectors of the denoiser, a full res GI reads it
  const char *halfMotionVectorsName = nullptr; // a half res GI reads it
  denoiser::DeclareInputs declareInputs;       // the gbuffer, the depth and the ordering of the trace
  denoiser::DeclareInputs declareDebugInputs;  // dev builds: the gbuffer the decoded debug view multiplies with
  // sets the view globals the trace shaders read, called before the trace: the view vectors, zn_zfar and world_view_pos
  eastl::fixed_function<32, void()> setShaderState;
  eastl::fixed_function<32, void()> resetShaderState; // undoes it after the trace
};

void initialize(bool half_res);
void teardown();

void set_second_chance_history(bool second_chance_history);

denoiser::ParamsHandle<FrameParams> create_frame_params(dafg::Registry registry);
// The trace node and the denoise node, see denoiser::make_gi_node. The denoiser validation overlay is a node
// of its own, registered while is_validation_layer_enabled: a change of it needs a new registration.
void register_nodes(const NodeParams &params, dag::Vector<dafg::NodeHandle> &nodes);

bool is_validation_layer_enabled();
bool is_second_chance_history_enabled();
void render_validation_layer();

} // namespace ptgi
