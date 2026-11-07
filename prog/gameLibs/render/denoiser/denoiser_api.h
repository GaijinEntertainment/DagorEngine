// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <drv/3d/dag_tex3d.h>
#include <drv/3d/dag_samplerHandle.h>
#include <3d/dag_resPtr.h>
#include <math/dag_Point2.h>
#include <math/dag_Point3.h>
#include <math/dag_Point4.h>
#include <math/dag_TMatrix4.h>
#include <math/integer/dag_IPoint2.h>
#include <EASTL/optional.h>
#include <startup/dag_globalSettings.h>
#include <EASTL/fixed_function.h>
#include <render/daFrameGraph/daFG.h>

namespace denoiser
{

// The resolution modes of the effects. The textures follow the daFG auto resolution of the prepare node
// (PrepareNodeParams::resolutionName), a half res effect has half of it, so a dynamic resolution resizes them.
struct Config
{
  bool useRayReconstruction = false;
  bool ptgiUseRayReconstruction = true;
  bool rtaoHalfRes = false;
  bool ptgiHalfRes = false;
  bool rtrHalfRes = false;
  void init(bool use_rr, bool ptgi_use_rr)
  {
    useRayReconstruction = use_rr;
    ptgiUseRayReconstruction = ptgi_use_rr;
  }
  void initRTR(bool is_half_res)
  {
    G_ASSERT(!useRayReconstruction || !is_half_res);
    rtrHalfRes = is_half_res && !useRayReconstruction;
  }
  void closeRTR() { rtrHalfRes = false; }
  void initRTAO(bool is_half_res)
  {
    G_ASSERT(!useRayReconstruction || !is_half_res);
    rtaoHalfRes = is_half_res && !useRayReconstruction;
  }
  void closeRTAO() { rtaoHalfRes = false; }
  void initPTGI(bool is_half_res)
  {
    G_ASSERT(!ptgiUseRayReconstruction || !is_half_res);
    ptgiHalfRes = is_half_res && !ptgiUseRayReconstruction;
  }
  void closePTGI() { ptgiHalfRes = false; }
  void closeAll() { *this = {}; }
};
extern Config resolution_config;

// the size of a texture: the resolution of the effect that writes it, dynamic resolution included
inline IPoint2 texture_size(const BaseTexture *tex)
{
  TextureInfo ti;
  tex->getinfo(ti);
  return IPoint2(ti.w, ti.h);
}

// The camera of one view a trace runs for. The scope lens of the camera in camera is a second view: the gbuffer
// holds the lens camera inside the lens area of the main view, a trace with a lens view dispatches once per view
// with the view state hooks of its node, and the trace shaders discard the pixels of the other view.
struct TraceView
{
  TMatrix4 projTm = TMatrix4::IDENT;
  Point3 viewPos = Point3::ZERO;
};

// The view state hooks of a trace node, called in a frame with a lens view
struct ViewStateHooks
{
  eastl::fixed_function<32, void(bool lens_view)> set; // applies the view state of the main view or of the lens before its trace
  eastl::fixed_function<32, void()> reset;             // restores the view state of the node after the lens trace
};

// Dispatches a trace once per view of the frame: the main view, then the lens with its view state around it
template <typename Trace> // void(const TraceView &view)
inline void trace_per_view(const ViewStateHooks &hooks, const TraceView &main_view, const eastl::optional<TraceView> &lens_view,
  Trace &&trace)
{
  if (lens_view && hooks.set)
    hooks.set(false);
  trace(main_view);
  if (!lens_view)
    return;
  if (hooks.set)
    hooks.set(true);
  trace(*lens_view);
  if (hooks.reset)
    hooks.reset();
}

enum class ReflectionMethod
{
  Reblur,
  Relax,
  // For a trace without noise: a blur by the reflection lobe, no history. The trace output is packed as for Relax, a
  // checkerboard in the left half, and hit distance 0 marks a texel without a ray.
  Blur,
};

bool is_available();
// A repeated call keeps the state of the first one, a change of the ray reconstruction flags needs a teardown first
void initialize(bool use_ray_reconstruction, bool ptgi_use_ray_reconstruction);
void teardown();

bool is_ray_reconstruction_enabled();
bool is_ptgi_ray_reconstruction_enabled();

int get_frame_number();

// The per frame data of the nodes: a game node creates the blob of the prepare node with create_prepare_params and
// fills it, the trace of an effect does the same for its denoise node. A blob with render false skips the node.

struct PrepareParams
{
  bool render = false;
  Point3 viewPos = Point3::ZERO;
  Point3 prevViewPos = Point3::ZERO;
  Point3 viewDir = Point3::ZERO;
  Point3 prevViewDir = Point3::ZERO;
  TMatrix4 viewItm = TMatrix4::IDENT;
  TMatrix4 projTm = TMatrix4::IDENT;
  TMatrix4 prevViewItm = TMatrix4::IDENT;
  TMatrix4 prevProjTm = TMatrix4::IDENT;
  Point2 jitter = Point2::ZERO;
  Point2 prevJitter = Point2::ZERO;
  Point3 motionMultiplier = Point3::ZERO;
  bool reset = false;
  // Anything closer than this to the camera is marked camera attached
  float denoiserCameraAttachedRadius = 0;
};

struct ShadowParams
{
  static constexpr int MAX_HISTORY_FRAME_NUM = 7;

  bool render = false;
  Point3 lightDirection = Point3(0, -1, 0);
  int maxStabilizedFrameNum = 5;
  Texture *csmTexture = nullptr;
  d3d::SamplerHandle csmSampler = d3d::INVALID_SAMPLER_HANDLE;
  Texture *vsmTexture = nullptr;
  d3d::SamplerHandle vsmSampler = d3d::INVALID_SAMPLER_HANDLE;
};

struct AOParams
{
  static constexpr int MAX_HISTORY_FRAME_NUM = 63;

  Point4 hitDistParams = Point4::ZERO;
  Point2 antilagSettings = Point2(4, 3);
  int maxStabilizedFrameNum = MAX_HISTORY_FRAME_NUM;
  bool performanceMode = true;
  bool checkerboard = true;
  bool render = false;
};

struct GIParams
{
  static constexpr int MAX_HISTORY_FRAME_NUM = 63;

  Point4 hitDistParams = Point4::ZERO;
  Point2 antilagSettings = Point2(4, 3);
  int maxStabilizedFrameNum = MAX_HISTORY_FRAME_NUM;
  bool antiFirefly = false;
  bool secondChanceDebug = false;
  bool performanceMode = true;
  bool checkerboard = true;
  bool render = false;
};

struct ReflectionParams
{
  static constexpr int REBLUR_MAX_HISTORY_FRAME_NUM = 63;
  static constexpr int RELAX_MAX_HISTORY_FRAME_NUM = 255;

  Point4 hitDistParams = Point4::ZERO;
  Point2 antilagSettings = Point2(4, 3);
  int maxStabilizedFrameNum = 31;
  int maxFastStabilizedFrameNum = 6;
  float blurRadius = 0;       // Blur: the largest radius, a fraction of the height, the output mips take a larger lobe
  float blurMaxRoughness = 1; // Blur: the trace shoots no ray from this roughness on, a checkerboard hole there has none either
  bool antiFirefly = true;
  bool performanceMode = true;
  bool checkerboard = true;
  bool render = false;
};

template <typename T>
using ParamsHandle = dafg::VirtualResourceHandle<T, false, false>;

ParamsHandle<PrepareParams> create_prepare_params(dafg::Registry registry);
ParamsHandle<ShadowParams> create_shadow_params(dafg::Registry registry);
ParamsHandle<AOParams> create_ao_params(dafg::Registry registry);
ParamsHandle<GIParams> create_gi_params(dafg::Registry registry);
ParamsHandle<ReflectionParams> create_reflection_params(dafg::Registry registry);

// The outputs of an effect: the node that traces it creates them, bound to the shader vars the trace writes, at the
// resolution of the effect. With ray reconstruction the trace output is the output. The handle is empty without the
// lib (the stub).
using OutputRwHandle = eastl::optional<dafg::VirtualResourceHandle<BaseTexture, true, false>>;
OutputRwHandle create_shadow_trace_output(dafg::Registry registry, const char *shader_var);
OutputRwHandle create_ao_trace_output(dafg::Registry registry, const char *shader_var);
OutputRwHandle create_gi_trace_output(dafg::Registry registry, const char *shader_var);
OutputRwHandle create_reflection_trace_output(dafg::Registry registry, const char *shader_var);
// A Blur output has the mip pyramid its reader samples for the lobes wider than the blur
OutputRwHandle create_reflection_output(dafg::Registry registry, const char *shader_var, ReflectionMethod method);
dafg::AutoResolutionRequest<2> reflection_resolution(dafg::Registry registry);

// The prepare outputs a trace reads, bound to its shader var. HalfRtr is the half resolution set made from the
// checkerboard or the selected depth (PrepareNodeParams).
enum class Inputs
{
  Full,
  Half,
  HalfRtr,
};
// The set of the reflections: the plain set of the resolution, or the rtr half set of a half res reflection that
// takes the checkerboard or the selected depth. The trace marks the view z of the set, the denoise reads it.
Inputs reflection_inputs(bool checkerboard_depth, bool smart_depth);
void read_normal_roughness(dafg::Registry registry, const char *shader_var, Inputs inputs);
void read_selected_depth(dafg::Registry registry, const char *shader_var);
// A trace node orders itself after the prepare node, which sets the frame state of every trace and denoise
void order_after_prepare(dafg::Registry registry);

// The shadow and the reflection trace mark the pixels their denoiser skips (sky, water) in the view z. A mark of one
// effect must not reach another denoiser, so a trace takes the view z over from the readers before it (a daFG
// rename): the AO and the GI denoise the prepare output, the shadow trace marks it next, the reflection trace last.
// PrepareNodeParams::shadowTrace tells the lib whether the shadow trace is in the graph.
void modify_view_z_for_shadow(dafg::Registry registry, const char *shader_var);
void modify_view_z_for_reflection(dafg::Registry registry, const char *shader_var, Inputs inputs);

// The output of an effect (the trace output with ray reconstruction) for the nodes that light with it: the read
// orders the node after the denoise, binds the output to a shader var when one is given, and keeps the prepare
// outputs the effect publishes through bindless (the GI depth, the reflection normal roughness) alive for the
// reader. The handle is empty without the lib (the stub).
// The reads are optional: a consumer registered while an effect was on keeps working after the effect's nodes are
// gone
using OutputHandle = eastl::optional<dafg::VirtualResourceHandle<const BaseTexture, true, true>>;
OutputHandle read_shadow_output(dafg::Registry registry);
OutputHandle read_ao_output(dafg::Registry registry);
OutputHandle read_gi_output(dafg::Registry registry);
OutputHandle read_reflection_output(dafg::Registry registry);
OutputHandle read_reflection_trace_output(dafg::Registry registry); // the output before the denoise, for a debug view
OutputHandle read_gi_validation(dafg::Registry registry);
OutputHandle read_reflection_validation(dafg::Registry registry);

#if DAGOR_DBGLEVEL > 0
// A copy of an output for a window or an overlay outside the frame graph: the output lives with the frame, the copy
// with the caller. Two copies alternate: a node writes one while ImGui draws the other, so the rect ImGui takes at its
// update is the rect of the content it draws at the end of the frame. A dynamic resolution changes the size every
// frame, so a copy only grows and the frame fills the top left rect of it: uvMax is the far corner of that rect.
struct DebugImage
{
  UniqueTex tex[2];
  IPoint2 rect[2] = {IPoint2::ZERO, IPoint2::ZERO};
  int shown = 0;
  unsigned nodeFrame = 0;
  // the debug node of the effect calls it every frame: the window shows the values of the effect while its nodes are
  // in the graph
  void nodeRan() { nodeFrame = dagor_frame_no(); }
  bool inGraph() const { return dagor_frame_no() <= nodeFrame + 1; } // the window runs before the node
  Texture *texture() const { return tex[shown].getTex2D(); }
  float aspect() const { return rect[shown].y > 0 ? float(rect[shown].x) / rect[shown].y : 1.f; }
  Point2 uvMax() const
  {
    if (!tex[shown])
      return Point2::ONE;
    TextureInfo ti;
    tex[shown]->getinfo(ti);
    return Point2(float(rect[shown].x) / ti.w, float(rect[shown].y) / ti.h);
  }
  void close()
  {
    tex[0].close();
    tex[1].close();
  }
};
// The next copy to write, grown to rect in the format (the TEXFMT and the TEXCF bits) and shown from now on: write it
// in the same node, its content is undefined. Null without a size.
Texture *fit_debug_image(DebugImage &dst, const IPoint2 &rect, uint32_t format, const char *name);
void copy_for_debug(BaseTexture *src, DebugImage &dst, const char *name);
#endif

// Game side requests of a node: the shader var bindings its passes read, ordering tokens
using DeclareInputs = eastl::fixed_function<32, void(dafg::Registry)>;

// The prepare shader loads the gbuffer and the motion buffer through its shader vars, and the depth sets
// through downsampled_close_depth_tex, downsampled_checkerboard_depth_tex and downsampled_far_depth_tex:
// declareInputs binds them.
struct PrepareNodeParams
{
  const char *resolutionName = "main_view"; // the daFG auto resolution of every denoiser texture
  bool halfResCloseSet = false;             // the half resolution view z and normal roughness, the inputs of a half res pass
  bool checkerboardSet = false;             // the rtr half resolution set from the checkerboard depth
  bool smartDepthSet = false;               // the rtr half resolution set and the selected depth from the far and the close depth
  bool shadowTrace = false;                 // the shadow trace is in the graph, see modify_view_z_for_shadow
  DeclareInputs declareInputs;
  // the prepare shader reads the view globals of the traces too: zn_zfar, the view vectors, world_view_pos
  eastl::fixed_function<32, void()> setShaderState;
};

struct ShadowNodeParams
{
  const char *motionVectorsName = nullptr;
};

struct AONodeParams
{
  const char *motionVectorsName = nullptr;     // a full res AO reads it
  const char *halfMotionVectorsName = nullptr; // a half res AO reads it
};

struct GINodeParams
{
  bool validation = false; // the node writes the debug view of the denoiser, read_gi_validation
  bool secondChanceHistory = false;
  const char *motionVectorsName = nullptr;
  const char *halfMotionVectorsName = nullptr;
};

struct ReflectionNodeParams
{
  ReflectionMethod method = ReflectionMethod::Reblur;
  bool checkerboardDepth = false; // a half res RTR reads the rtr half resolution set of the prepare node
  bool useSmartDepth = false;     // the same set from the selected depth
  bool validation = false;
  const char *motionVectorsName = nullptr;
  const char *halfMotionVectorsName = nullptr; // a half res RTR without the rtr set reads it
};

// Register the prepare node before the effect nodes: it starts the frame plan the effect nodes add to. The lib nodes
// register at the root namespace, so the reads of the outputs resolve there from a node of any namespace. Register
// the prepare node again with the effect nodes: its registration resets what theirs record, the shadow trace and
// the reflection set the readers of the outputs follow.
dafg::NodeHandle make_prepare_node(const PrepareNodeParams &params);
dafg::NodeHandle make_shadow_node(const ShadowNodeParams &params);
dafg::NodeHandle make_ao_node(const AONodeParams &params);
dafg::NodeHandle make_gi_node(const GINodeParams &params);
dafg::NodeHandle make_reflection_node(const ReflectionNodeParams &params);
// The alias nodes of the prepare outputs the GI and the reflection denoises publish through bindless, registered
// with their effect nodes: a reader keeps the texture alive by the alias, which outlives a change of the effect set.
dafg::NodeHandle make_gi_view_z_node();
dafg::NodeHandle make_reflection_normal_roughness_node();

} // namespace denoiser
