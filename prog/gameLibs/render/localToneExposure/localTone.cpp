// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/localToneExposure/localTone_api.h>

#include <drv/3d/dag_sampler.h>
#include <drv/3d/dag_texFlags.h>
#include <EASTL/fixed_string.h>
#include <EASTL/optional.h>
#include <EASTL/string.h>
#include <math/dag_check_nan.h>
#include <math/dag_color.h>
#include <math/dag_mathBase.h>
#include <math/integer/dag_IPoint2.h>
#include <perfMon/dag_cpuFreq.h>
#include <perfMon/dag_statDrv.h>
#include <render/antialiasing.h>
#include <render/daFrameGraph/daFG.h>
#include <shaders/dag_computeShaders.h>
#include <shaders/dag_postFxRenderer.h>
#include <shaders/dag_shaders.h>
#include <shaders/dag_shaderVariableInfo.h>
#include <startup/dag_globalSettings.h>
#include <util/dag_console.h>

namespace local_tone
{
namespace
{
constexpr const char *NAME_SPACE = "local_tone";
constexpr const char *DOWNSAMPLE_SHADER = "frame_bloom_downsample";
constexpr const char *POOL_SHADER = "local_tone_pool";
constexpr const char *BLUR_SHADER = "local_tone_blur";

constexpr int BLOCK = 8;
constexpr int POOL_GROUP = 4;
// The textures are sized for MIN_POOL.
constexpr int MIN_POOL = 4;
// The last static variant of local_tone_pool.
constexpr int MAX_POOL = 6;
// The pooled height of the fitted look, 1440p.
constexpr float POOLED_HEIGHT = 180.f;
constexpr float BLUR_TRUNCATE = 2.5f;
constexpr int MAX_TAP_RADIUS = 128;
constexpr float MIN_SIGMA_TEXELS = 0.1f;
constexpr float MAX_GAIN_STOPS = 16.f;

using Reason = eastl::fixed_string<char, 192, true>;

#define GLOBAL_VARS_LIST              \
  VAR(local_tone_enabled)             \
  VAR(local_tone_blur)                \
  VAR(local_tone_gain)                \
  VAR(local_tone_curve)               \
  VAR(local_tone_zones)               \
  VAR(local_tone_sky_exposure_indoor) \
  VAR(local_tone_kernel)              \
  VAR(local_tone_size)                \
  VAR(local_tone_view_size)           \
  VAR(local_tone_pool_size)           \
  VAR(local_tone_blur_pass)           \
  VAR(bloom_src_uv_transform)

#define VAR(a) ShaderVariableInfo a##VarId(#a, true);
GLOBAL_VARS_LIST
#undef VAR

// Inputs can vanish before the nodes update.
bool input_found = false;
// Set by the first node each frame.
bool frame_valid = false;
IPoint2 low_size;
Reason reported;

struct Kernel
{
  float sigma = 1.f;
  float invRange = 0.f;
  float historySeconds = 0.f;
  int radius = 1;
};

bool finite4(const Color4 &c) { return check_finite(c.r) && check_finite(c.g) && check_finite(c.b) && check_finite(c.a); }

bool read_kernel(const IPoint2 &low, Kernel &kernel, Reason &reason)
{
  const Color4 &blur = local_tone_blurVarId.get_float4();
  const Color4 &gain = local_tone_gainVarId.get_float4();
  const float sigma = blur.r * low.y;
  if (!finite4(blur) || sigma < MIN_SIGMA_TEXELS || ceilf(BLUR_TRUNCATE * sigma) > MAX_TAP_RADIUS)
    reason.sprintf("local_tone_blur.x %g: the blur sigma is a part of the pooled height, %g to %g here", blur.r,
      MIN_SIGMA_TEXELS / low.y, MAX_TAP_RADIUS / (BLUR_TRUNCATE * low.y));
  else if (blur.g < 0.f)
    reason.sprintf("local_tone_blur.y %g: the range sigma is 0 or more stops", blur.g);
  else if (blur.b < 0.f)
    reason.sprintf("local_tone_blur.z %g: the history time constant is 0 or more seconds", blur.b);
  else if (!finite4(gain) || gain.r > gain.g || gain.a < 0.f || gain.a > 1.f)
    reason.sprintf("local_tone_gain %g, %g, %g, %g: use min <= max stops, a finite strength and a highlight keep of 0 to 1", gain.r,
      gain.g, gain.b, gain.a);
  else if (max(fabsf(gain.r), fabsf(gain.g)) * fabsf(gain.b) > MAX_GAIN_STOPS)
    reason.sprintf("local_tone_gain %g, %g, %g: max(|min|, |max|) * |strength| is above %g stops", gain.r, gain.g, gain.b,
      MAX_GAIN_STOPS);
  else if (!finite4(local_tone_curveVarId.get_float4()) || !finite4(local_tone_zonesVarId.get_float4()) ||
           !check_finite(local_tone_sky_exposure_indoorVarId.get_float()))
    reason = "local_tone_curve, local_tone_zones or local_tone_sky_exposure_indoor has a value that is not finite";
  else
  {
    kernel.sigma = sigma;
    kernel.invRange = blur.g > 0.f ? 1.f / blur.g : 0.f;
    kernel.historySeconds = blur.b;
    kernel.radius = int(ceilf(BLUR_TRUNCATE * sigma));
    return true;
  }
  return false;
}

float frame_seconds()
{
  static uint32_t frameId = 0;
  static int64_t ticks = 0;
  static float seconds = 0.f;
  const uint32_t id = dagor_get_global_frame_id();
  if (id != frameId)
  {
    const int64_t now = ref_time_ticks();
    seconds = ticks ? ref_time_delta_to_usec(now - ticks) * 1e-6f : 0.f;
    ticks = now;
    frameId = id;
  }
  return seconds;
}

void report(const Reason &reason)
{
  if (reason == reported)
    return;
  reported = reason;
  if (!reason.empty())
    console::error("local tone: off: %s", reason.c_str());
}

auto low_resolution(dafg::Registry registry, const char *resolution, int block = 1)
{
  return registry.getResolution<2>(resolution, 1.f / (2 * MIN_POOL * block));
}

// False keeps the global exposure.
bool begin_frame(const IPoint2 &frame, bool reset_history)
{
  const int pool = clamp(int(roundf(frame.y / (2 * POOLED_HEIGHT))), MIN_POOL, MAX_POOL);
  const IPoint2 low = frame / (2 * pool);
  Reason reason;
  Kernel kernel;
  frame_valid = read_kernel(low, kernel, reason) && input_found;
  report(reason);
  if (!frame_valid)
  {
    local_tone_sizeVarId.set_int4(0, 0, 0, 0);
    return false;
  }
  // The history is on the last grid.
  const bool newGrid = low != low_size;
  low_size = low;
  const float seconds = frame_seconds();
  const float history = reset_history || newGrid || kernel.historySeconds <= 0.f ? 0.f : expf(-seconds / kernel.historySeconds);
  local_tone_sizeVarId.set_int4(frame.x, frame.y, low.x, low.y);
  local_tone_pool_sizeVarId.set_int(pool);
  local_tone_kernelVarId.set_float4(kernel.sigma, kernel.invRange, history, float(kernel.radius));
  return true;
}

void bind_read(dafg::Registry registry, const char *name, const char *shader_var)
{
  registry.readTexture(name).atStage(dafg::Stage::COMPUTE).bindToShaderVar(shader_var);
}

void bind_history(dafg::Registry registry, const char *name, const char *shader_var)
{
  registry.readTextureHistory(name).atStage(dafg::Stage::COMPUTE).bindToShaderVar(shader_var);
}

void bind_created(dafg::Registry registry, const char *name, const char *shader_var,
  const decltype(dafg::Texture2dCreateInfo::resolution) &size, dafg::History history = dafg::History::No)
{
  registry.createTexture2d(name, {TEXFMT_R32F | TEXCF_UNORDERED, size})
    .withHistory(history)
    .atStage(dafg::Stage::COMPUTE)
    .bindToShaderVar(shader_var);
}

dafg::NodeHandle make_downsample_node(dafg::NameSpace ns, const char *frame, const char *resolution)
{
  return ns.registerNode("downsample", DAFG_PP_NODE_SRC, [frame = eastl::string(frame), resolution](dafg::Registry registry) {
    auto frameHndl =
      registry.root().readTexture(frame.c_str()).atStage(dafg::Stage::PS).bindToShaderVar("blur_src_tex").optional().handle();
    d3d::SamplerInfo smpInfo;
    smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Clamp;
    registry.create("downsample_sampler")
      .blob<d3d::SamplerHandle>(d3d::request_sampler(smpInfo))
      .bindToShaderVar("blur_src_tex_samplerstate");
    registry.createTexture2d("downsampled", {TEXCF_RTARGET | TEXFMT_R11G11B10F, registry.getResolution<2>(resolution, 0.5f)});
    registry.requestRenderPass().color({"downsampled"});
    return [frameHndl, renderer = PostFxRenderer(DOWNSAMPLE_SHADER)]() {
      input_found = frameHndl.get() != nullptr;
      if (!input_found)
        return;
      TIME_D3D_PROFILE(local_tone_downsample);
      bloom_src_uv_transformVarId.set_float4(1, 1, 0, 0);
      renderer.render();
    };
  });
}

dafg::NodeHandle make_pool_node(dafg::NameSpace ns, const char *downsampled_frame, const char *resolution)
{
  return ns.registerNode("pool", DAFG_PP_NODE_SRC,
    [halfFrame = eastl::string(downsampled_frame ? downsampled_frame : "downsampled"), resolution,
      game_downsample = downsampled_frame != nullptr](dafg::Registry registry) {
      const auto frameRes = registry.getResolution<2>(resolution);
      auto halfFrameHndl = (game_downsample ? registry.root() : registry.currNameSpace())
                             .readTexture(halfFrame.c_str())
                             .atStage(dafg::Stage::COMPUTE)
                             .bindToShaderVar("local_tone_half_frame")
                             .optional()
                             .handle();
      bind_created(registry, "pooled", "local_tone_target", low_resolution(registry, resolution));
      auto applyCtxHndl = registry.root().readBlob<render::antialiasing::ApplyContext>("aa_apply_context").optional().handle();
      return [halfFrameHndl, applyCtxHndl, frameRes, game_downsample, shader = ComputeShader(POOL_SHADER)]() {
        if (game_downsample)
          input_found = halfFrameHndl.get() != nullptr;
        const render::antialiasing::ApplyContext *applyCtx = applyCtxHndl.get();
        if (!begin_frame(frameRes.get(), applyCtx && applyCtx->resetHistory))
          return;
        TIME_D3D_PROFILE(local_tone_pool);
        shader.dispatchGroups((low_size.x + POOL_GROUP - 1) / POOL_GROUP, (low_size.y + POOL_GROUP - 1) / POOL_GROUP, 1);
      };
    });
}

dafg::NodeHandle make_blur_h_node(dafg::NameSpace ns, const char *resolution)
{
  return ns.registerNode("blur_h", DAFG_PP_NODE_SRC, [resolution](dafg::Registry registry) {
    bind_read(registry, "pooled", "local_tone_guide");
    bind_created(registry, "blur_h", "local_tone_target", low_resolution(registry, resolution));
    bind_created(registry, "sums", "local_tone_sums", low_resolution(registry, resolution, BLOCK).roundingUp());
    return [shader = ComputeShader(BLUR_SHADER)]() {
      if (!frame_valid)
        return;
      TIME_D3D_PROFILE(local_tone_blur_h);
      shader.dispatchThreads(low_size.x, low_size.y, 1);
    };
  });
}

dafg::NodeHandle make_blur_v_node(dafg::NameSpace ns, const char *motion_vectors, const char *resolution, const char *view_resolution)
{
  return ns.registerNode("blur_v", DAFG_PP_NODE_SRC,
    [motionVectors = eastl::string(motion_vectors ? motion_vectors : ""), resolution, view_resolution](dafg::Registry registry) {
      const auto lowRes = low_resolution(registry, resolution);
      bind_read(registry, "pooled", "local_tone_guide");
      bind_read(registry, "blur_h", "local_tone_source");
      bind_read(registry, "sums", "local_tone_sums");
      bind_history(registry, "mean", "local_tone_history");
      bind_history(registry, "frame_mean", "local_tone_frame_history");
      bind_created(registry, "mean", "local_tone_target", lowRes, dafg::History::ClearZeroOnFirstFrame);
      bind_created(registry, "frame_mean", "local_tone_frame_mean", IPoint2(1, 1), dafg::History::ClearZeroOnFirstFrame);
      eastl::optional<dafg::VirtualResourceHandle<const BaseTexture, true, true>> motionHndl;
      if (!motionVectors.empty())
        motionHndl = registry.root()
                       .readTexture(motionVectors.c_str())
                       .atStage(dafg::Stage::COMPUTE)
                       .bindToShaderVar("local_tone_motion")
                       .optional()
                       .handle();
      const auto viewRes = registry.getResolution<2>(view_resolution);
      return [motionHndl, viewRes, shader = ComputeShader(BLUR_SHADER)]() {
        if (!frame_valid)
          return;
        TIME_D3D_PROFILE(local_tone_blur_v);
        const IPoint2 view = viewRes.get();
        local_tone_view_sizeVarId.set_int4(view.x, view.y, motionHndl && motionHndl->get() ? 1 : 0, 0);
        local_tone_blur_passVarId.set_int(1);
        shader.dispatchThreads(low_size.x, low_size.y, 1);
        local_tone_blur_passVarId.set_int(0);
      };
    });
}
} // namespace

bool is_enabled() { return local_tone_enabledVarId.get_int() != 0; }

bool has_shaders(const Inputs &inputs)
{
  return shader_exists(POOL_SHADER) && shader_exists(BLUR_SHADER) && (inputs.downsampledFrame || shader_exists(DOWNSAMPLE_SHADER));
}

void register_nodes(const Inputs &inputs, dag::Vector<dafg::NodeHandle> &nodes)
{
  dafg::NameSpace ns = dafg::root() / NAME_SPACE;
  if (!inputs.downsampledFrame)
    nodes.emplace_back(make_downsample_node(ns, inputs.frame, inputs.frameResolution));
  nodes.emplace_back(make_pool_node(ns, inputs.downsampledFrame, inputs.frameResolution));
  nodes.emplace_back(make_blur_h_node(ns, inputs.frameResolution));
  nodes.emplace_back(make_blur_v_node(ns, inputs.motionVectors, inputs.frameResolution, inputs.viewResolution));
}

void read_hook_inputs(dafg::Registry registry, dafg::Stage stage)
{
  auto ns = registry.root() / NAME_SPACE;
  ns.readTexture("pooled").atStage(stage).bindToShaderVar("local_tone_guide").optional();
  ns.readTexture("mean").atStage(stage).bindToShaderVar("local_tone_mean").optional();
  ns.readTexture("frame_mean").atStage(stage).bindToShaderVar("local_tone_frame_mean").optional();
}
} // namespace local_tone
