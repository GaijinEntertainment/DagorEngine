// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <math/dag_TMatrix4.h>
#include <math/integer/dag_IPoint2.h>
#include <dag/dag_vector.h>
#include <EASTL/fixed_function.h>
#include <render/daFrameGraph/daFG.h>
#include <render/denoiser/denoiser_api.h>

namespace bvh
{
struct Context;
using ContextId = Context *;
} // namespace bvh

namespace rtr
{

// The per frame data of the trace: a game node creates the blob with create_frame_params and fills it. That
// node is one of the root namespace, since the lib nodes register at the root and read the blob there.
struct FrameParams
{
  bvh::ContextId ctxId = nullptr;
  TMatrix4 projTm = TMatrix4::IDENT;
  eastl::optional<denoiser::TraceView> lensView; // the scope lens camera while the lens is rendered
  bool rtShadow = false;                         // the hit points trace their sun shadow
  bool randomizeRays = false;                    // the probe rays
  bool render = false;
};

struct NodeParams
{
  denoiser::ReflectionMethod method = denoiser::ReflectionMethod::Reblur;
  // A half res trace and denoise read the rtr half set of the denoiser prepare, made from the checkerboard depth or
  // from the selected depth, in place of the plain half set
  bool checkerboardDepth = false;
  bool smartDepth = false;
  const char *motionVectorsName = nullptr;     // the motion vectors of the denoiser, a full res RTR reads it
  const char *halfMotionVectorsName = nullptr; // a half res RTR without the checkerboard reads it
  denoiser::DeclareInputs declareInputs;       // the gbuffer, the depth and the ordering of the trace
  denoiser::DeclareInputs declareProbeInputs;
  // sets the view globals the trace shaders read, called before the trace: the view vectors, zn_zfar and world_view_pos
  eastl::fixed_function<32, void()> setShaderState;
  eastl::fixed_function<32, void()> resetShaderState; // undoes it after the trace
  denoiser::ViewStateHooks viewState;                 // the hooks of a frame with a lens view
};

void initialize(bool half_res, bool checkerboard, bool path_traced);
void teardown();
void set_resolution(const IPoint2 &max_rendering_resolution);
void set_dynamic_resolution(const IPoint2 &rendering_resolution);

void set_performance_mode(bool performance_mode);

void set_water_params();
void set_rtr_hit_distance_params();
void set_ray_limit_params(float ray_limit_coeff, float ray_limit_power, float probe_treshold, bool use_rtr_probes);
void set_use_anti_firefly(bool use_anti_firefly_);
// The largest radius of the blur of the Blur denoiser method, a fraction of the view height. The reader takes a wider
// lobe from the mips of the output (getRTRWeighted of rtr_use.dshl)
void set_blur_radius(float blur_radius_);

denoiser::ParamsHandle<FrameParams> create_frame_params(dafg::Registry registry);
// The trace node and the denoise node, see denoiser::make_reflection_node. The denoiser validation overlay is a
// node of its own, registered while is_validation_layer_enabled: a change of it needs a new registration.
void register_nodes(const NodeParams &params, dag::Vector<dafg::NodeHandle> &nodes);

bool is_validation_layer_enabled();
// Over the frame: the denoiser validation layer, or with the Blur method the overlay picked in the RTR window
void render_validation_layer();
void render_probes();

} // namespace rtr
