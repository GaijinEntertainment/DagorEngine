// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <drv/3d/dag_rwResource.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_info.h>
#include <3d/dag_resPtr.h>
#include <drv/3d/dag_tex3d.h>
#include <EASTL/unique_ptr.h>
#include <render/downsampleDepth.h>
#include <shaders/dag_shaders.h>
#include <shaders/dag_postFxRenderer.h>
#include <shaders/dag_computeShaders.h>
#include <shaders/depth_hierarchy_inc.hlsli>
#include <shaders/dag_overrideStates.h>
#include <perfMon/dag_statDrv.h>
#include <3d/dag_textureIDHolder.h>

#define GLOBAL_VARS_LIST                  \
  VAR(downsample_depth_from)              \
  VAR(downsample_depth_from_samplerstate) \
  VAR(downsample_from)                    \
  VAR(downsample_to)                      \
  VAR(downsample_uv_transform)            \
  VAR(downsample_uv_transformi)

#define GLOBAL_VARS_LIST_OPTIONAL                 \
  VAR(downsample_depth_type)                      \
  VAR(downsample_op)                              \
  VAR(required_mip_count)                         \
  VAR(work_group_count)                           \
  VAR(downsample_closest_depth_from)              \
  VAR(downsample_closest_depth_from_samplerstate) \
  VAR(downsampled_depth_mip_count)                \
  VAR(depth_format_target)                        \
  VAR(has_motion_vectors)                         \
  VAR(has_checkerboard_depth)                     \
  VAR(has_normal)

#define VAR(a) static int a##VarId = -1;
GLOBAL_VARS_LIST
GLOBAL_VARS_LIST_OPTIONAL
#undef VAR

namespace downsample_depth
{

enum // same as in shaders
{
  DTYPE_GBUF_FAR = 0,
  DTYPE_GBUF_FAR_CLOSE = 1,
  DTYPE_MIP_FAR = 2,
  DTYPE_MIP_FAR_CLOSE = 3,
  DTYPE_OTHER = 4,
};

enum
{
  DOWNSAMPLE_OP_MIN = 0,
  DOWNSAMPLE_OP_MAX = 1
};

enum
{
  NO_DEPTH_FORMAT = 0,
  FAR_DEPTH_FORMAT = 1,
  CLOSE_DEPTH_FORMAT = 2,
  CHECKER_DEPTH_FORMAT = 3,
};

static PostFxRenderer downsampleDepth;
static eastl::unique_ptr<ComputeShaderElement> downsampleDepthCompute;
static eastl::unique_ptr<ComputeShaderElement> downsampleDepthWaveCompute;
static UniqueBuf atomicCountersBuf;
static shaders::UniqueOverrideStateId zFuncAlwaysStateId;

static const int MAX_CS_SUPPORTED_MIPS = 12;

void close()
{
  downsampleDepth.clear();
  downsampleDepthCompute.reset();
  downsampleDepthWaveCompute.reset();
  atomicCountersBuf.close();
  zFuncAlwaysStateId.reset();
}
void init(const char *ps_name, const char *wave_cs_name, const char *cs_name)
{
  downsampleDepth.init(ps_name);
  if (cs_name)
  {
    downsampleDepthCompute.reset(new_compute_shader(cs_name, true /* optional */));
  }
  if (wave_cs_name)
  {
    downsampleDepthWaveCompute.reset(new_compute_shader(wave_cs_name, true /* optional */));
  }

  if (downsampleDepthWaveCompute)
  {
    atomicCountersBuf = dag::buffers::create_ua_sr_structured(sizeof(uint32_t), 2, "DownsamplerAtomicCounters");

    d3d::zero_rwbufi(atomicCountersBuf.getBuf());
  }

  shaders::OverrideState zFuncState;
  zFuncState.set(shaders::OverrideState::Z_FUNC);
  zFuncState.zFunc = CMPF_ALWAYS;
  zFuncAlwaysStateId = shaders::overrides::create(zFuncState);

#define VAR(a) a##VarId = get_shader_variable_id(#a);
  GLOBAL_VARS_LIST
#undef VAR
#define VAR(a) a##VarId = get_shader_variable_id(#a, true);
  GLOBAL_VARS_LIST_OPTIONAL
#undef VAR

  {
    d3d::SamplerInfo smpInfo;
    smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Clamp;
    ShaderGlobal::set_sampler(downsample_depth_from_samplerstateVarId, d3d::request_sampler(smpInfo));
  }
  {
    d3d::SamplerInfo smpInfo;
    smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Clamp;
    smpInfo.filter_mode = d3d::FilterMode::Point;
    smpInfo.mip_map_mode = d3d::MipMapMode::Point;
    ShaderGlobal::set_sampler(downsample_closest_depth_from_samplerstateVarId, d3d::request_sampler(smpInfo));
  }
}

void downsample(const ManagedTex &from_depth, int w, int h, const ManagedTex &far_depth, const ManagedTex &close_depth,
  const ManagedTex &far_normals, const ManagedTex &motion_vectors, const ManagedTex &checkerboard_depth, bool external_barriers)
{
  TextureIDPair close_depth_pair(close_depth.getTex2D(), close_depth.getTexId());
  TextureIDPair far_normals_pair(far_normals.getTex2D(), far_normals.getTexId());
  TextureIDPair motion_vectors_pair(motion_vectors.getTex2D(), motion_vectors.getTexId());
  TextureIDPair checkerboard_depth_pair(checkerboard_depth.getTex2D(), checkerboard_depth.getTexId());

  downsample(TextureIDPair(from_depth.getTex2D(), from_depth.getTexId()), w, h,
    TextureIDPair(far_depth.getTex2D(), far_depth.getTexId()), close_depth ? &close_depth_pair : nullptr,
    far_normals ? &far_normals_pair : nullptr, motion_vectors ? &motion_vectors_pair : nullptr,
    checkerboard_depth ? &checkerboard_depth_pair : nullptr, external_barriers);
}

void downsample(const TextureIDPair &from_depth, int w, int h, const TextureIDPair &far_depth, const TextureIDPair *close_depth,
  const TextureIDPair *far_normals, const TextureIDPair *motion_vectors, const TextureIDPair *checkerboard_depth,
  bool external_barriers)
{
  if (downsampleDepthWaveCompute && (far_depth.getTex2D()->level_count() - 1) <= MAX_CS_SUPPORTED_MIPS &&
      (!close_depth || (close_depth->getTex2D()->level_count() - 1) <= MAX_CS_SUPPORTED_MIPS))
  {
    downsampleWithWaveIntin(from_depth, w, h, far_depth, close_depth, far_normals, motion_vectors, checkerboard_depth,
      external_barriers);
  }
  else
  {
    downsamplePS(from_depth, w, h, &far_depth, close_depth, far_normals, motion_vectors, checkerboard_depth, external_barriers);
  }
}

void downsamplePS(const TextureIDPair &from_depth, int w, int h, const TextureIDPair *far_depth, const TextureIDPair *close_depth,
  const TextureIDPair *far_normals, const TextureIDPair *motion_vectors, const TextureIDPair *checkerboard_depth,
  bool external_barriers, const Point4 &source_uv_transform)
{
  downsamplePS(from_depth, w, h, {far_depth, far_depth != nullptr}, close_depth, far_normals, motion_vectors, checkerboard_depth,
    external_barriers, source_uv_transform);
}

static bool check_uav(const TextureIDPair *tex)
{
  if (!tex || !tex->getTex2D())
    return true;

  TextureInfo info;
  tex->getTex2D()->getinfo(info);

  return (info.cflg & TEXCF_UNORDERED) != 0;
}

void downsamplePS(const TextureIDPair &from_depth, int w, int h, dag::Span<const TextureIDPair> far_depth_array,
  const TextureIDPair *close_depth, const TextureIDPair *far_normals, const TextureIDPair *motion_vectors,
  const TextureIDPair *checkerboard_depth, bool external_barriers, const Point4 &source_uv_transform)
{
  const TextureIDPair *far_depth_mip0 = far_depth_array.size() ? &far_depth_array[0] : nullptr;

  G_ASSERT(far_depth_mip0 || close_depth || checkerboard_depth);
  G_ASSERTF(!close_depth || close_depth && far_depth_mip0, "DownsamplePS: `close_depth` only downsample has not implemented!");
  G_ASSERT_RETURN(from_depth.getTex(), );

  int savedHasMotionVectors = ShaderGlobal::get_int(has_motion_vectorsVarId);
#if DAGOR_DBGLEVEL > 0
  if (motion_vectors && savedHasMotionVectors == 0)
    logwarn("DownsamplePS: motion_vectors were provided but ShaderVar has_motion_vectors = 0. motion_vectors will not downsample.");
#endif

  bool useCompute = downsampleDepthCompute && check_uav(far_depth_mip0) && check_uav(close_depth) && check_uav(far_normals) &&
                    check_uav(motion_vectors) && check_uav(checkerboard_depth);

  G_ASSERT(downsampleDepth.getElem());
  if (!downsampleDepth.getElem())
    return;

  // TODO: external_barriers should also have the effect of managing render targets, something that framegraph is also designed for.
  SCOPE_RENDER_TARGET;

  int downsample_type = DTYPE_OTHER;
  if (far_depth_mip0)
  {
    downsample_type = close_depth ? DTYPE_GBUF_FAR_CLOSE : DTYPE_GBUF_FAR;
  }

  ShaderGlobal::set_int(downsample_depth_typeVarId, downsample_type);
  ShaderGlobal::set_int(has_checkerboard_depthVarId, checkerboard_depth ? 1 : 0);
  ShaderGlobal::set_int(has_normalVarId, far_normals ? 1 : 0);
  ShaderGlobal::set_int(has_motion_vectorsVarId, savedHasMotionVectors && motion_vectors ? 1 : 0);

  auto isDepthBuffer = [](TextureIDPair tex) {
    TextureInfo depthInfo;
    tex.getTex2D()->getinfo(depthInfo, 0);
    return (depthInfo.cflg & TEXFMT_MASK) >= TEXFMT_FIRST_DEPTH && (depthInfo.cflg & TEXFMT_MASK) <= TEXFMT_LAST_DEPTH;
  };

  Texture *depthTex = nullptr;
  int depthFormatTarget = NO_DEPTH_FORMAT;
  for (const auto &tex : {eastl::pair(far_depth_mip0, FAR_DEPTH_FORMAT), eastl::pair(close_depth, CLOSE_DEPTH_FORMAT),
         eastl::pair(checkerboard_depth, CHECKER_DEPTH_FORMAT)})
  {
    if (tex.first && isDepthBuffer(tex.first->getTex2D()))
    {
      G_ASSERT(!depthTex && "DownsamplePS: Only one target can have depth format.");
      depthTex = tex.first->getTex2D();
      depthFormatTarget = tex.second;
    }
  }

  ShaderGlobal::set_int(depth_format_targetVarId, depthFormatTarget);
  if (useCompute)
  {
    d3d::set_rwtex(STAGE_CS, 0, far_depth_mip0 ? far_depth_mip0->getTex2D() : nullptr, 0, 0);
    d3d::set_rwtex(STAGE_CS, 1, close_depth ? close_depth->getTex2D() : nullptr, 0, 0);
    d3d::set_rwtex(STAGE_CS, 2, checkerboard_depth ? checkerboard_depth->getTex2D() : nullptr, 0, 0);
    d3d::set_rwtex(STAGE_CS, 3, far_normals ? far_normals->getTex2D() : nullptr, 0, 0);
    d3d::set_rwtex(STAGE_CS, 4, motion_vectors ? motion_vectors->getTex2D() : nullptr, 0, 0);
  }
  else
  {
    if (depthTex)
      shaders::overrides::set(zFuncAlwaysStateId);

    d3d::set_render_target({depthTex ? depthTex : nullptr, 0}, DepthAccess::RW,
      {{far_depth_mip0 && depthFormatTarget != FAR_DEPTH_FORMAT ? far_depth_array[0].getTex2D() : nullptr, 0},
        {close_depth && depthFormatTarget != CLOSE_DEPTH_FORMAT ? close_depth->getTex2D() : nullptr, 0},
        {checkerboard_depth && depthFormatTarget != CHECKER_DEPTH_FORMAT ? checkerboard_depth->getTex2D() : nullptr, 0},
        {far_normals ? far_normals->getTex2D() : nullptr, 0}, {motion_vectors ? motion_vectors->getTex2D() : nullptr, 0}});
  }

  TextureInfo fdi;
  from_depth.getTex2D()->getinfo(fdi, 0);

  ShaderGlobal::set_color4(downsample_fromVarId, w, h, 0, 0);
  ShaderGlobal::set_color4(downsample_toVarId, w >> 1, h >> 1, 0, 0);
  ShaderGlobal::set_texture(downsample_depth_fromVarId, from_depth.getId());
  ShaderGlobal::set_color4(downsample_uv_transformVarId, source_uv_transform);
  ShaderGlobal::set_int4(downsample_uv_transformiVarId, source_uv_transform.z * fdi.w, source_uv_transform.w * fdi.h, 0, 0);

  // TODO: This barrier can be managed externally (external_barriers)
  d3d::resource_barrier({from_depth.getTex(), RB_RO_SRV | RB_STAGE_COMPUTE | RB_STAGE_PIXEL, 0, 0});

  if (!useCompute)
  {
    TIME_D3D_PROFILE(clear);
    bool canDiscard = !d3d::get_driver_desc().issues.hasRenderPassClearDataRace;
    d3d::clearview(canDiscard ? CLEAR_DISCARD : CLEAR_ZBUFFER, 0, 0.f, 0);
  }

  {
    TIME_D3D_PROFILE(first_pass);
    if (useCompute)
    {
      for (int smpIx = 0; smpIx < 16; smpIx++)
        d3d::set_tex(STAGE_CS, smpIx, nullptr);

      downsampleDepthCompute->dispatchThreads(w >> 1, h >> 1, 1);
      for (int ix = 0; ix < 5; ++ix)
        d3d::set_rwtex(STAGE_CS, ix, nullptr, 0, 0);
    }
    else
      downsampleDepth.render();
  }

  ShaderGlobal::set_int(has_motion_vectorsVarId, 0);
  ShaderGlobal::set_int(has_checkerboard_depthVarId, 0);
  ShaderGlobal::set_int(has_normalVarId, 0);

  ShaderGlobal::set_color4(downsample_uv_transformVarId, 1, 1, 0, 0);
  ShaderGlobal::set_int4(downsample_uv_transformiVarId, 0, 0, 0, 0);

  const bool farHasDepthFormat = depthFormatTarget == FAR_DEPTH_FORMAT;
  const int far_depth_mip_count =
    (far_depth_array.size() && !farHasDepthFormat) ? far_depth_array[0].getTex2D()->level_count() : far_depth_array.size();
  const int close_depth_mip_count = close_depth ? close_depth->getTex2D()->level_count() : 0;

  G_ASSERTF(close_depth_mip_count <= 1 || close_depth_mip_count <= far_depth_mip_count,
    "DownsamplePS: Downsampling close depth more steps than far depth has not implemented.");

  ResourceBarrier srcStage = useCompute ? RB_SOURCE_STAGE_COMPUTE : RB_SOURCE_STAGE_PIXEL;
  if (far_depth_mip_count > 1)
  {
    G_ASSERTF(depthFormatTarget != CLOSE_DEPTH_FORMAT,
      "DownsamplePS: Mipmap chain with `close_depth` using depth format has not implemented");

    if (!farHasDepthFormat)
    {
      ShaderGlobal::set_texture(downsample_depth_fromVarId, far_depth_array[0].getId());
    }

    if (close_depth)
    {
      ShaderGlobal::set_texture(downsample_closest_depth_fromVarId, close_depth->getId());
    }

    ShaderGlobal::set_int(downsampled_depth_mip_countVarId, far_depth_mip_count);
    ShaderGlobal::set_int(depth_format_targetVarId, farHasDepthFormat ? FAR_DEPTH_FORMAT : NO_DEPTH_FORMAT);

    for (int i = 1; i < far_depth_mip_count; ++i) // mips
    {
      char mipName[] = "mip_0";
      mipName[4] += i;
      TIME_D3D_PROFILE_NAME(first_pass, mipName);

      const bool closeDepthEnabled = i < close_depth_mip_count;
      ShaderGlobal::set_int(downsample_depth_typeVarId, closeDepthEnabled ? DTYPE_MIP_FAR_CLOSE : DTYPE_MIP_FAR);
      ShaderGlobal::set_color4(downsample_fromVarId, w >> i, h >> i, 0, 0);
      ShaderGlobal::set_color4(downsample_toVarId, w >> (i + 1), h >> (i + 1), 0, 0);

      auto getFarRenderTarget = [farHasDepthFormat, far_depth_array](unsigned mip_level) -> RenderTarget {
        return farHasDepthFormat ? RenderTarget{far_depth_array[mip_level].getTex2D(), 0}
                                 : RenderTarget{far_depth_array[0].getTex2D(), mip_level};
      };
      RenderTarget farDepthSource = getFarRenderTarget(unsigned(i - 1));
      RenderTarget farDepthTarget = getFarRenderTarget(unsigned(i));

      // TODO: Stop using RB_RO_COPY_SOURCE and both RB_STAGE_PIXEL | RB_STAGE_COMPUTE to prepare for external use
      // if(external_barriers)
      d3d::resource_barrier({farDepthSource.tex, RB_RO_SRV | srcStage | RB_STAGE_PIXEL | RB_STAGE_COMPUTE | RB_RO_COPY_SOURCE,
        farDepthSource.mip_level, 1});
      if (farHasDepthFormat)
      {
        ShaderGlobal::set_texture(downsample_depth_fromVarId, far_depth_array[unsigned(i - 1)].getId()); // farDepthSource texture ID
      }
      else
      {
        farDepthSource.tex->texmiplevel(farDepthSource.mip_level, farDepthSource.mip_level);
      }

      if (closeDepthEnabled)
      {
        d3d::resource_barrier({close_depth->getTex2D(), RB_RO_SRV | srcStage | RB_STAGE_PIXEL | RB_STAGE_COMPUTE, unsigned(i - 1), 1});
        close_depth->getTex2D()->texmiplevel(i - 1, i - 1);
      }

      if (useCompute)
      {
        d3d::set_rwtex(STAGE_CS, 0, farDepthTarget.tex, 0, farDepthTarget.mip_level);
        if (closeDepthEnabled)
          d3d::set_rwtex(STAGE_CS, 1, close_depth->getTex2D(), 0, unsigned(i));
        downsampleDepthCompute->dispatchThreads(w >> (i + 1), h >> (i + 1), 1);
      }
      else
      {
        d3d::set_render_target(farHasDepthFormat ? farDepthTarget : RenderTarget{}, DepthAccess::RW,
          {!farHasDepthFormat ? farDepthTarget : RenderTarget{},
            {closeDepthEnabled ? close_depth->getTex2D() : nullptr, unsigned(i)}});
        d3d::clearview(CLEAR_DISCARD, 0, 0.f, 0);
        downsampleDepth.render();
      }
    }

    if (useCompute)
    {
      d3d::set_rwtex(STAGE_CS, 0, nullptr, 0, 0);
      d3d::set_rwtex(STAGE_CS, 1, nullptr, 0, 0);
    }
  }

  // Preserve state of `has_motion_vectors`.
  ShaderGlobal::set_int(has_motion_vectorsVarId, savedHasMotionVectors);

  shaders::overrides::reset();
  ShaderGlobal::set_texture(downsample_depth_fromVarId, BAD_TEXTUREID);

  if (close_depth)
  {
    close_depth->getTex2D()->texmiplevel(-1, -1);
  }
  if (checkerboard_depth)
  {
    checkerboard_depth->getTex2D()->texmiplevel(-1, -1);
  }
  if (far_depth_array.size())
  {
    // TODO: This barrier can be managed externally (external_barriers)
    if (!farHasDepthFormat)
    {
      far_depth_array[0].getTex2D()->texmiplevel(-1, -1);
      d3d::resource_barrier({far_depth_array[0].getTex2D(),
        RB_RO_SRV | srcStage | RB_STAGE_PIXEL | RB_STAGE_COMPUTE | RB_RO_COPY_SOURCE, unsigned(far_depth_mip_count - 1), 1});
    }
    else
    {
      d3d::resource_barrier({far_depth_array[far_depth_mip_count - 1].getTex2D(),
        RB_RO_SRV | srcStage | RB_STAGE_PIXEL | RB_STAGE_COMPUTE | RB_RO_COPY_SOURCE, 0, 1});
    }
  }

  if (close_depth && !external_barriers)
    d3d::resource_barrier(
      {close_depth->getTex2D(), RB_RO_SRV | srcStage | RB_STAGE_PIXEL | RB_STAGE_COMPUTE, unsigned(close_depth_mip_count - 1), 1});
  if (far_normals && !external_barriers)
    d3d::resource_barrier({far_normals->getTex2D(), RB_RO_SRV | srcStage | RB_STAGE_PIXEL, 0, 1});
  if (motion_vectors && !external_barriers)
    d3d::resource_barrier({motion_vectors->getTex2D(), RB_RO_SRV | srcStage | RB_STAGE_PIXEL, 0, 1});
  if (checkerboard_depth && !external_barriers)
    d3d::resource_barrier({checkerboard_depth->getTex2D(), RB_RO_SRV | srcStage | RB_STAGE_PIXEL | RB_STAGE_VERTEX, 0, 1});
}

void generate_depth_mips(const TextureIDPair &tex)
{
  SCOPE_RENDER_TARGET;
  int savedHasMotionVectors = ShaderGlobal::get_int(has_motion_vectorsVarId);

  ShaderGlobal::set_int(downsample_depth_typeVarId, DTYPE_MIP_FAR);
  ShaderGlobal::set_int(depth_format_targetVarId, NO_DEPTH_FORMAT);
  ShaderGlobal::set_int(has_normalVarId, 0);
  ShaderGlobal::set_int(has_checkerboard_depthVarId, 0);
  ShaderGlobal::set_int(has_motion_vectorsVarId, 0);
  ShaderGlobal::set_texture(downsample_depth_fromVarId, tex.getId());

  for (int i = 1; i < tex.getTex2D()->level_count(); ++i)
  {
    TextureInfo info;
    tex.getTex2D()->getinfo(info, i);
    ShaderGlobal::set_color4(downsample_fromVarId, info.w >> i, info.h >> i, 0, 0);
    tex.getTex2D()->texmiplevel(i - 1, i - 1);
    d3d::set_render_target(tex.getTex2D(), i);
    d3d::clearview(CLEAR_DISCARD_TARGET, 0, 0.f, 0);
    downsampleDepth.render();
  }

  tex.getTex2D()->texmiplevel(-1, -1);

  // Preserve state of `has_motion_vectors`.
  ShaderGlobal::set_int(has_motion_vectorsVarId, savedHasMotionVectors);
}

void generate_depth_mips(const TextureIDPair *depth_mips, int depth_mip_count)
{
  SCOPE_RENDER_TARGET;
  int savedHasMotionVectors = ShaderGlobal::get_int(has_motion_vectorsVarId);

  shaders::overrides::set(zFuncAlwaysStateId);
  ShaderGlobal::set_int(downsample_depth_typeVarId, DTYPE_MIP_FAR);
  ShaderGlobal::set_int(depth_format_targetVarId, FAR_DEPTH_FORMAT);
  ShaderGlobal::set_int(has_normalVarId, 0);
  ShaderGlobal::set_int(has_checkerboard_depthVarId, 0);
  ShaderGlobal::set_int(has_motion_vectorsVarId, 0);
  d3d::set_render_target(nullptr, 0);

  TextureInfo info;
  depth_mips[0].getTex2D()->getinfo(info, 0);

  for (int i = 1; i < depth_mip_count; ++i)
  {
    ShaderGlobal::set_color4(downsample_fromVarId, info.w >> (i - 1), info.h >> (i - 1), 0, 0);
    ShaderGlobal::set_texture(downsample_depth_fromVarId, depth_mips[i - 1].getId());
    d3d::resource_barrier({depth_mips[i - 1].getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL | RB_STAGE_COMPUTE | RB_RO_COPY_SOURCE, 0, 1});
    d3d::set_render_target(nullptr, 0);
    d3d::set_depth(depth_mips[i].getTex2D(), DepthAccess::RW);
    d3d::clearview(CLEAR_DISCARD_ZBUFFER, 0, 0.f, 0);
    downsampleDepth.render();
  }
  d3d::resource_barrier(
    {depth_mips[depth_mip_count - 1].getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL | RB_STAGE_COMPUTE | RB_RO_COPY_SOURCE, 0, 1});

  shaders::overrides::reset();

  // Preserve state of `has_motion_vectors`.
  ShaderGlobal::set_int(has_motion_vectorsVarId, savedHasMotionVectors);
}

void downsampleWithWaveIntin(const TextureIDPair &from_depth, int w, int h, const TextureIDPair &far_depth,
  const TextureIDPair *close_depth, const TextureIDPair *far_normals, const TextureIDPair *motion_vectors,
  const TextureIDPair *checkerboard_depth, bool external_barriers)
{
  G_ASSERT(downsampleDepthWaveCompute);
#if DAGOR_DBGLEVEL > 0
  {
    TextureInfo farDepthInfo;
    far_depth.getTex2D()->getinfo(farDepthInfo, 0);
    if (!(farDepthInfo.cflg & TEXCF_UNORDERED))
      logerr("downsampleHierarchy requires TEXCF_UNORDERED flag.\n"
             "Please, add this flag to '%s' texture.\n"
             "NOTE: Use downsamplePS for downsampling to depth textures.",
        far_depth.getTex2D()->getTexName());
  }
#endif

  SCOPE_RENDER_TARGET;
  int savedHasMotionVectors = ShaderGlobal::get_int(has_motion_vectorsVarId);
  int downsample_type = close_depth ? DTYPE_GBUF_FAR_CLOSE : DTYPE_GBUF_FAR;


  { // Write the first downsampled mip level
    ShaderGlobal::set_int(downsample_depth_typeVarId, downsample_type);
    ShaderGlobal::set_int(depth_format_targetVarId, NO_DEPTH_FORMAT);
    ShaderGlobal::set_int(has_normalVarId, far_normals ? 1 : 0);
    ShaderGlobal::set_int(has_checkerboard_depthVarId, checkerboard_depth ? 1 : 0);
    ShaderGlobal::set_int(has_motion_vectorsVarId, savedHasMotionVectors && motion_vectors ? 1 : 0);
    d3d::set_render_target(far_depth.getTex2D(), 0);
    d3d::set_render_target(1, close_depth ? close_depth->getTex2D() : NULL, 0);
    d3d::set_render_target(2, checkerboard_depth ? checkerboard_depth->getTex2D() : NULL, 0);
    d3d::set_render_target(3, far_normals ? far_normals->getTex2D() : NULL, 0);
    d3d::set_render_target(4, motion_vectors ? motion_vectors->getTex2D() : NULL, 0);

    ShaderGlobal::set_color4(downsample_fromVarId, w, h, 0, 0);
    ShaderGlobal::set_texture(downsample_depth_fromVarId, from_depth.getId());
    d3d::clearview(CLEAR_DISCARD_TARGET, 0, 0.f, 0);
    downsampleDepth.render(); // first pass
  }

  w /= 2;
  h /= 2;

  const int workGroupX = (w + DEPTH_HIERARCHY_DISPATCH_SIZE_X - 1) / DEPTH_HIERARCHY_DISPATCH_SIZE_X;
  const int workGroupY = (h + DEPTH_HIERARCHY_DISPATCH_SIZE_Y - 1) / DEPTH_HIERARCHY_DISPATCH_SIZE_Y;
  ShaderGlobal::set_int(work_group_countVarId, workGroupX * workGroupY);

  //
  // Downsample far depth
  //
  const int farMipsCount = far_depth.getTex2D()->level_count() - 1;
  if (farMipsCount > 0)
  {
    d3d::resource_barrier({far_depth.getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL | RB_STAGE_COMPUTE | RB_RO_COPY_SOURCE, 0, 1});
    far_depth.getTex2D()->texmiplevel(0, 0);
    d3d::set_tex(STAGE_CS, 0, far_depth.getTex2D());

    ShaderGlobal::set_int(downsampled_depth_mip_countVarId, farMipsCount + 1);
    for (int i = 1; i < far_depth.getTex2D()->level_count(); ++i)
    {
      d3d::set_rwtex(STAGE_CS, i - 1, far_depth.getTex2D(), 0, i);
    }
    for (int i = farMipsCount; i < MAX_CS_SUPPORTED_MIPS; ++i)
    {
      d3d::set_rwtex(STAGE_CS, i, nullptr, 0, 0);
    }
    d3d::set_rwbuffer(STAGE_CS, 12, atomicCountersBuf.getBuf());
    ShaderGlobal::set_int(downsample_opVarId, DOWNSAMPLE_OP_MIN);
    ShaderGlobal::set_int(required_mip_countVarId, farMipsCount);

    downsampleDepthWaveCompute->dispatch(workGroupX, workGroupY, 1);
  }
  //
  // Downsample near depth
  //
  if (close_depth && close_depth->getTex2D()->level_count() > 1)
  {
    d3d::resource_barrier({close_depth->getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL | RB_STAGE_COMPUTE, 0, 1});
    d3d::resource_barrier({atomicCountersBuf.getBuf(), RB_NONE});
    close_depth->getTex2D()->texmiplevel(0, 0);
    d3d::set_tex(STAGE_CS, 0, close_depth->getTex2D());

    const int closeMipsCount = close_depth->getTex2D()->level_count() - 1;
    for (int i = 1; i < close_depth->getTex2D()->level_count(); ++i)
    {
      d3d::set_rwtex(STAGE_CS, i - 1, close_depth->getTex2D(), 0, i);
    }
    for (int i = closeMipsCount; i < MAX_CS_SUPPORTED_MIPS; ++i)
    {
      d3d::set_rwtex(STAGE_CS, i, nullptr, 0, 0);
    }
    ShaderGlobal::set_int(downsample_opVarId, DOWNSAMPLE_OP_MAX);
    ShaderGlobal::set_int(required_mip_countVarId, closeMipsCount);
    downsampleDepthWaveCompute->dispatch(workGroupX, workGroupY, 1);
  }

  // Preserve state of `has_motion_vectors`.
  ShaderGlobal::set_int(has_motion_vectorsVarId, savedHasMotionVectors);

  d3d::set_tex(STAGE_CS, 0, nullptr);
  d3d::set_rwbuffer(STAGE_CS, MAX_CS_SUPPORTED_MIPS, nullptr);
  for (int i = 0; i < MAX_CS_SUPPORTED_MIPS; ++i)
  {
    d3d::set_rwtex(STAGE_CS, i, nullptr, 0, 0);
  }

  far_depth.getTex2D()->texmiplevel(-1, -1);
  if (farMipsCount > 0)
  {
    d3d::resource_barrier( //
      {far_depth.getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL | RB_STAGE_COMPUTE | RB_RO_COPY_SOURCE, 1, (unsigned)farMipsCount});
  }
  if (close_depth)
  {
    close_depth->getTex2D()->texmiplevel(-1, -1);
    if (!external_barriers && close_depth->getTex2D()->level_count() > 1)
    {
      d3d::resource_barrier( //
        {close_depth->getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL | RB_STAGE_COMPUTE, 1,
          (unsigned)close_depth->getTex2D()->level_count() - 1u});
    }
  }
  if (far_normals)
    d3d::resource_barrier({far_normals->getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 1});
  if (motion_vectors)
    d3d::resource_barrier({motion_vectors->getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 1});
  if (checkerboard_depth)
  {
    checkerboard_depth->getTex2D()->texmiplevel(-1, -1);
    if (!external_barriers)
      d3d::resource_barrier({checkerboard_depth->getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL | RB_STAGE_VERTEX, 0, 1});
  }
  d3d::resource_barrier({atomicCountersBuf.getBuf(), RB_FLUSH_UAV | RB_STAGE_COMPUTE | RB_SOURCE_STAGE_COMPUTE});
}
}; // namespace downsample_depth
