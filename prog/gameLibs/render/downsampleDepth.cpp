// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <drv/3d/dag_rwResource.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_driverDesc.h>
#include <drv/3d/dag_viewScissor.h>
#include <3d/dag_resPtr.h>
#include <drv/3d/dag_tex3d.h>
#include <EASTL/unique_ptr.h>
#include <render/downsampleDepth.h>
#include <shaders/dag_shaders.h>
#include <shaders/dag_postFxRenderer.h>
#include <shaders/dag_computeShaders.h>
#include <shaders/depth_hierarchy_inc.hlsli>
#include <shaders/dag_overrideStates.h>
#include <shaders/dag_dynamicResolutionStcode.h>
#include <perfMon/dag_statDrv.h>
#include <3d/dag_textureIDHolder.h>
#include <math/integer/dag_IPoint2.h>

#define GLOBAL_VARS_LIST       \
  VAR(downsample_depth_from)   \
  VAR(downsample_from)         \
  VAR(downsample_to)           \
  VAR(downsample_uv_transform) \
  VAR(downsample_uv_transformi)

#define GLOBAL_VARS_LIST_OPTIONAL                 \
  VAR(downsample_depth_type)                      \
  VAR(downsample_op)                              \
  VAR(required_mip_count)                         \
  VAR(work_group_count)                           \
  VAR(downsample_closest_depth_from)              \
  VAR(downsample_closest_depth_from_samplerstate) \
  VAR(downsampled_depth_mip_count)                \
  VAR(depth_buffer_size)                          \
  VAR(depth_format_target)                        \
  VAR(has_motion_vectors)                         \
  VAR(has_checkerboard_depth)                     \
  VAR(has_normal)                                 \
  VAR(normal_repacking_needed)

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
    atomicCountersBuf =
      dag::buffers::create_ua_sr_structured(sizeof(uint32_t), 2, "DownsamplerAtomicCounters", d3d::buffers::Init::No, RESTAG_TARGET);

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
    smpInfo.filter_mode = d3d::FilterMode::Point;
    smpInfo.mip_map_mode = d3d::MipMapMode::Point;
    ShaderGlobal::set_sampler(downsample_closest_depth_from_samplerstateVarId, d3d::request_sampler(smpInfo));
  }
}

void downsample(const ManagedTex &from_depth, int w, int h, const ManagedTex &far_depth, const ManagedTex &close_depth,
  const ManagedTex &far_normals, const ManagedTex &normal_gbuf, const ManagedTex &motion_vectors, const ManagedTex &checkerboard_depth,
  bool external_barriers)
{
  downsample(from_depth.getTex2D(), w, h, far_depth.getTex2D(), close_depth.getTex2D(), far_normals.getTex2D(), normal_gbuf.getTex2D(),
    motion_vectors.getTex2D(), checkerboard_depth.getTex2D(), external_barriers);
}

void downsample(BaseTexture *from_depth, int w, int h, BaseTexture *far_depth, BaseTexture *close_depth, BaseTexture *far_normals,
  BaseTexture *normal_gbuf, BaseTexture *motion_vectors, BaseTexture *checkerboard_depth, bool external_barriers)
{
  if (downsampleDepthWaveCompute && (far_depth->level_count() - 1) <= MAX_CS_SUPPORTED_MIPS &&
      (!close_depth || (close_depth->level_count() - 1) <= MAX_CS_SUPPORTED_MIPS))
  {
    downsampleWithWaveIntin(from_depth, w, h, far_depth, close_depth, far_normals, normal_gbuf, motion_vectors, checkerboard_depth,
      external_barriers);
  }
  else
  {
    downsamplePS(from_depth, w, h, far_depth, close_depth, far_normals, normal_gbuf, motion_vectors, checkerboard_depth,
      external_barriers);
  }
}

void downsamplePS(BaseTexture *from_depth, int w, int h, BaseTexture *far_depth, BaseTexture *close_depth, BaseTexture *far_normals,
  BaseTexture *normal_gbuf, BaseTexture *motion_vectors, BaseTexture *checkerboard_depth, bool external_barriers,
  const Point4 &source_uv_transform, const DynRes *dynamic_resolution)
{
  downsamplePS(from_depth, w, h, {&far_depth, far_depth != nullptr}, close_depth, far_normals, normal_gbuf, motion_vectors,
    checkerboard_depth, external_barriers, source_uv_transform, dynamic_resolution);
}

static bool check_uav(BaseTexture *tex)
{
  if (!tex)
    return true;

  TextureInfo info;
  tex->getinfo(info);

  return (info.cflg & TEXCF_UNORDERED) != 0;
}

void downsamplePS(BaseTexture *from_depth, int w, int h, dag::Span<BaseTexture *> far_depth_array, BaseTexture *close_depth,
  BaseTexture *far_normals, BaseTexture *normal_gbuf, BaseTexture *motion_vectors, BaseTexture *checkerboard_depth,
  bool external_barriers, const Point4 &source_uv_transform, const DynRes *dynamic_resolution)
{
  BaseTexture *far_depth_mip0 = far_depth_array.size() ? far_depth_array[0] : nullptr;

  G_ASSERT(far_depth_mip0 || close_depth || checkerboard_depth);
  G_ASSERTF(!close_depth || close_depth && far_depth_mip0, "DownsamplePS: `close_depth` only downsample has not implemented!");
  G_ASSERT_RETURN(from_depth, );

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

  auto isDepthBuffer = [](Texture *tex) {
    TextureInfo depthInfo;
    tex->getinfo(depthInfo, 0);
    return (depthInfo.cflg & TEXFMT_MASK) >= TEXFMT_FIRST_DEPTH && (depthInfo.cflg & TEXFMT_MASK) <= TEXFMT_LAST_DEPTH;
  };

  Texture *depthTex = nullptr;
  int depthFormatTarget = NO_DEPTH_FORMAT;
  for (const auto &tex : {eastl::pair(far_depth_mip0, FAR_DEPTH_FORMAT), eastl::pair(close_depth, CLOSE_DEPTH_FORMAT),
         eastl::pair(checkerboard_depth, CHECKER_DEPTH_FORMAT)})
  {
    if (tex.first && isDepthBuffer(tex.first))
    {
      G_ASSERT(!depthTex && "DownsamplePS: Only one target can have depth format.");
      depthTex = tex.first;
      depthFormatTarget = tex.second;
    }
  }

  ShaderGlobal::set_int(downsample_depth_typeVarId, downsample_type);
  ShaderGlobal::set_int(depth_format_targetVarId, depthFormatTarget);
  TextureInfo fdi;
  from_depth->getinfo(fdi, 0);

  ShaderGlobal::set_float4(downsample_fromVarId, w, h, 0, 0);
  ShaderGlobal::set_float4(downsample_toVarId, w >> 1, h >> 1, 0, 0);
  ShaderGlobal::set_texture(downsample_depth_fromVarId, from_depth);

  // TODO: This barrier can be managed externally (external_barriers)
  d3d::resource_barrier({from_depth, RB_RO_SRV | RB_STAGE_COMPUTE | RB_STAGE_PIXEL, 0, 0});

  {
    STATE_GUARD(ShaderGlobal::set_int(has_checkerboard_depthVarId, VALUE), checkerboard_depth ? 1 : 0, 0);
    STATE_GUARD(ShaderGlobal::set_int(has_normalVarId, VALUE), far_normals ? 1 : 0, 0);
    STATE_GUARD(ShaderGlobal::set_int(has_motion_vectorsVarId, VALUE), savedHasMotionVectors && motion_vectors ? 1 : 0, 0);
    STATE_GUARD(ShaderGlobal::set_int(normal_repacking_neededVarId, VALUE), normal_gbuf ? 1 : 0, 0);
    STATE_GUARD(ShaderGlobal::set_float4(downsample_uv_transformVarId, VALUE), Point4(source_uv_transform), Point4(1, 1, 0, 0));
    STATE_GUARD(ShaderGlobal::set_int4(downsample_uv_transformiVarId, VALUE),
      IPoint4(source_uv_transform.z * fdi.w, source_uv_transform.w * fdi.h, 0, 0), IPoint4(0, 0, 0, 0));

    int mw = (dynamic_resolution ? dynamic_resolution->dynamicResolution.x : w) / 2;
    int mh = (dynamic_resolution ? dynamic_resolution->dynamicResolution.y : h) / 2;

    if (useCompute)
    {
      STATE_GUARD_NULLPTR(d3d::set_rwtex(STAGE_CS, 0, VALUE, 0, 0), far_depth_mip0);
      STATE_GUARD_NULLPTR(d3d::set_rwtex(STAGE_CS, 1, VALUE, 0, 0), close_depth);
      STATE_GUARD_NULLPTR(d3d::set_rwtex(STAGE_CS, 2, VALUE, 0, 0), checkerboard_depth);
      STATE_GUARD_NULLPTR(d3d::set_rwtex(STAGE_CS, 3, VALUE, 0, 0), far_normals);
      STATE_GUARD_NULLPTR(d3d::set_rwtex(STAGE_CS, 4, VALUE, 0, 0), motion_vectors);
      STATE_GUARD_NULLPTR(d3d::set_rwtex(STAGE_CS, 5, VALUE, 0, 0), normal_gbuf);
      for (int smpIx = 0; smpIx < 16; smpIx++)
        d3d::set_tex(STAGE_CS, smpIx, nullptr);

      downsampleDepthCompute->dispatchThreads(mw, mh, 1);
    }
    else
    {
      STATE_GUARD_NULLPTR(d3d::set_rwtex(STAGE_PS, 5, VALUE, 0, 0), normal_gbuf);

      if (depthTex)
        shaders::overrides::set(zFuncAlwaysStateId);

      d3d::set_render_target({depthTex ? depthTex : nullptr, 0}, DepthAccess::RW,
        {{far_depth_mip0 && depthFormatTarget != FAR_DEPTH_FORMAT ? far_depth_array[0] : nullptr, 0},
          {close_depth && depthFormatTarget != CLOSE_DEPTH_FORMAT ? close_depth : nullptr, 0},
          {checkerboard_depth && depthFormatTarget != CHECKER_DEPTH_FORMAT ? checkerboard_depth : nullptr, 0},
          {far_normals ? far_normals : nullptr, 0}, {motion_vectors ? motion_vectors : nullptr, 0}});

      d3d::setview(0, 0, mw, mh, 0, 1);
      d3d::setscissor(0, 0, mw, mh);

      {
        TIME_D3D_PROFILE(clear);
        bool canDiscard = !d3d::get_driver_desc().issues.hasRenderPassClearDataRace;
        d3d::clearview(canDiscard ? CLEAR_DISCARD : CLEAR_ZBUFFER, 0, 0.f, 0);
      }

      {
        TIME_D3D_PROFILE(first_pass)
        downsampleDepth.render();
      }
    }
  }

  const bool farHasDepthFormat = depthFormatTarget == FAR_DEPTH_FORMAT;
  const int far_depth_mip_count =
    (far_depth_array.size() && !farHasDepthFormat) ? far_depth_array[0]->level_count() : far_depth_array.size();
  const int close_depth_mip_count = close_depth ? close_depth->level_count() : 0;

  G_ASSERTF(close_depth_mip_count <= 1 || close_depth_mip_count <= far_depth_mip_count,
    "DownsamplePS: Downsampling close depth more steps than far depth has not implemented.");

  ResourceBarrier srcStage = useCompute ? RB_SOURCE_STAGE_COMPUTE : RB_SOURCE_STAGE_PIXEL;
  if (far_depth_mip_count > 1)
  {
    G_ASSERTF(depthFormatTarget != CLOSE_DEPTH_FORMAT,
      "DownsamplePS: Mipmap chain with `close_depth` using depth format has not implemented");

    if (!farHasDepthFormat)
    {
      ShaderGlobal::set_texture(downsample_depth_fromVarId, far_depth_array[0]);
    }

    if (close_depth)
    {
      ShaderGlobal::set_texture(downsample_closest_depth_fromVarId, close_depth);
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
      ShaderGlobal::set_float4(downsample_fromVarId, w >> i, h >> i, 0, 0);
      ShaderGlobal::set_float4(downsample_toVarId, w >> (i + 1), h >> (i + 1), 0, 0);

      auto getFarRenderTarget = [farHasDepthFormat, far_depth_array](unsigned mip_level) -> RenderTarget {
        return farHasDepthFormat ? RenderTarget{far_depth_array[mip_level], 0} : RenderTarget{far_depth_array[0], mip_level};
      };
      RenderTarget farDepthSource = getFarRenderTarget(unsigned(i - 1));
      RenderTarget farDepthTarget = getFarRenderTarget(unsigned(i));

      // TODO: Stop using RB_RO_COPY_SOURCE and both RB_STAGE_PIXEL | RB_STAGE_COMPUTE to prepare for external use
      // if(external_barriers)
      d3d::resource_barrier({farDepthSource.tex, RB_RO_SRV | srcStage | RB_STAGE_PIXEL | RB_STAGE_COMPUTE | RB_RO_COPY_SOURCE,
        farDepthSource.mip_level, 1});
      if (farHasDepthFormat)
      {
        ShaderGlobal::set_texture(downsample_depth_fromVarId, far_depth_array[unsigned(i - 1)]); // farDepthSource texture ID
      }
      else
      {
        farDepthSource.tex->texmiplevel(farDepthSource.mip_level, farDepthSource.mip_level);
      }

      if (closeDepthEnabled)
      {
        d3d::resource_barrier({close_depth, RB_RO_SRV | srcStage | RB_STAGE_PIXEL | RB_STAGE_COMPUTE, unsigned(i - 1), 1});
        close_depth->texmiplevel(i - 1, i - 1);
      }

      int mw = (dynamic_resolution ? (int)dynamic_resolution->dynamicResolution.x : w) >> (i + 1);
      int mh = (dynamic_resolution ? (int)dynamic_resolution->dynamicResolution.y : h) >> (i + 1);

      if (useCompute)
      {
        d3d::set_rwtex(STAGE_CS, 0, farDepthTarget.tex, 0, farDepthTarget.mip_level);
        if (closeDepthEnabled)
          d3d::set_rwtex(STAGE_CS, 1, close_depth, 0, unsigned(i));
        downsampleDepthCompute->dispatchThreads(mw, mh, 1);
      }
      else
      {
        d3d::set_render_target(farHasDepthFormat ? farDepthTarget : RenderTarget{}, DepthAccess::RW,
          {!farHasDepthFormat ? farDepthTarget : RenderTarget{}, {closeDepthEnabled ? close_depth : nullptr, unsigned(i)}});
        d3d::setview(0, 0, mw, mh, 0, 1);
        d3d::setscissor(0, 0, mw, mh);
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
  ShaderGlobal::set_texture(downsample_closest_depth_fromVarId, BAD_TEXTUREID);

  if (close_depth)
  {
    close_depth->texmiplevel(-1, -1);
  }
  if (checkerboard_depth)
  {
    checkerboard_depth->texmiplevel(-1, -1);
  }
  if (far_depth_array.size())
  {
    // TODO: This barrier can be managed externally (external_barriers)
    if (!farHasDepthFormat)
    {
      far_depth_array[0]->texmiplevel(-1, -1);
      d3d::resource_barrier({far_depth_array[0], RB_RO_SRV | srcStage | RB_STAGE_PIXEL | RB_STAGE_COMPUTE | RB_RO_COPY_SOURCE,
        unsigned(far_depth_mip_count - 1), 1});
    }
    else
    {
      d3d::resource_barrier({far_depth_array[far_depth_mip_count - 1],
        RB_RO_SRV | srcStage | RB_STAGE_PIXEL | RB_STAGE_COMPUTE | RB_RO_COPY_SOURCE, 0, 1});
    }
  }

  if (close_depth && !external_barriers)
    d3d::resource_barrier(
      {close_depth, RB_RO_SRV | srcStage | RB_STAGE_PIXEL | RB_STAGE_COMPUTE, unsigned(close_depth_mip_count - 1), 1});
  if (far_normals && !external_barriers)
    d3d::resource_barrier({far_normals, RB_RO_SRV | srcStage | RB_STAGE_PIXEL, 0, 1});
  if (motion_vectors && !external_barriers)
    d3d::resource_barrier({motion_vectors, RB_RO_SRV | srcStage | RB_STAGE_PIXEL, 0, 1});
  if (checkerboard_depth && !external_barriers)
    d3d::resource_barrier({checkerboard_depth, RB_RO_SRV | srcStage | RB_STAGE_PIXEL | RB_STAGE_VERTEX, 0, 1});
  if (normal_gbuf && !external_barriers)
    d3d::resource_barrier({normal_gbuf, RB_RO_SRV | srcStage | RB_STAGE_PIXEL, 0, 1});
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
    ShaderGlobal::set_float4(downsample_fromVarId, info.w >> i, info.h >> i, 0, 0);
    tex.getTex2D()->texmiplevel(i - 1, i - 1);
    d3d::set_render_target({}, DepthAccess::RW, {{tex.getTex2D(), static_cast<uint32_t>(i), 0}});
    d3d::clearview(CLEAR_DISCARD_TARGET, 0, 0.f, 0);
    downsampleDepth.render();
  }

  tex.getTex2D()->texmiplevel(-1, -1);

  ShaderGlobal::set_texture(downsample_depth_fromVarId, nullptr);
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

  TextureInfo info;
  depth_mips[0].getTex2D()->getinfo(info, 0);

  for (int i = 1; i < depth_mip_count; ++i)
  {
    ShaderGlobal::set_float4(downsample_fromVarId, info.w >> (i - 1), info.h >> (i - 1), 0, 0);
    ShaderGlobal::set_texture(downsample_depth_fromVarId, depth_mips[i - 1].getId());
    d3d::resource_barrier({depth_mips[i - 1].getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL | RB_STAGE_COMPUTE | RB_RO_COPY_SOURCE, 0, 1});
    d3d::set_render_target({depth_mips[i].getTex2D(), 0, 0}, DepthAccess::RW, {});
    d3d::clearview(CLEAR_DISCARD_ZBUFFER, 0, 0.f, 0);
    downsampleDepth.render();
  }
  d3d::resource_barrier(
    {depth_mips[depth_mip_count - 1].getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL | RB_STAGE_COMPUTE | RB_RO_COPY_SOURCE, 0, 1});

  shaders::overrides::reset();
  ShaderGlobal::set_texture(downsample_depth_fromVarId, nullptr);

  // Preserve state of `has_motion_vectors`.
  ShaderGlobal::set_int(has_motion_vectorsVarId, savedHasMotionVectors);
}

void downsampleWithWaveIntin(BaseTexture *from_depth, int w, int h, BaseTexture *far_depth, BaseTexture *close_depth,
  BaseTexture *far_normals, BaseTexture *normal_gbuf, BaseTexture *motion_vectors, BaseTexture *checkerboard_depth,
  bool external_barriers)
{
  G_ASSERT(downsampleDepthWaveCompute);
#if DAGOR_DBGLEVEL > 0
  {
    TextureInfo farDepthInfo;
    far_depth->getinfo(farDepthInfo, 0);
    if (!(farDepthInfo.cflg & TEXCF_UNORDERED))
      logerr("downsampleHierarchy requires TEXCF_UNORDERED flag.\n"
             "Please, add this flag to '%s' texture.\n"
             "NOTE: Use downsamplePS for downsampling to depth textures.",
        far_depth->getTexName());
  }
#endif

  SCOPE_RENDER_TARGET;
  int savedHasMotionVectors = ShaderGlobal::get_int(has_motion_vectorsVarId);
  int downsample_type = close_depth ? DTYPE_GBUF_FAR_CLOSE : DTYPE_GBUF_FAR;


  { // Write the first downsampled mip level
    ShaderGlobal::set_int(downsample_depth_typeVarId, downsample_type);
    ShaderGlobal::set_int(depth_format_targetVarId, NO_DEPTH_FORMAT);
    ShaderGlobal::set_int(has_normalVarId, far_normals ? 1 : 0);
    ShaderGlobal::set_int(normal_repacking_neededVarId, normal_gbuf ? 1 : 0);
    ShaderGlobal::set_int(has_checkerboard_depthVarId, checkerboard_depth ? 1 : 0);
    ShaderGlobal::set_int(has_motion_vectorsVarId, savedHasMotionVectors && motion_vectors ? 1 : 0);
    d3d::set_render_target({}, DepthAccess::RW,
      {{far_depth, 0, 0}, {close_depth, 0, 0}, {checkerboard_depth, 0, 0}, {far_normals, 0, 0}, {motion_vectors, 0, 0}});
    d3d::set_rwtex(STAGE_PS, 5, normal_gbuf, 0, 0);
    ShaderGlobal::set_float4(downsample_fromVarId, w, h, 0, 0);
    ShaderGlobal::set_texture(downsample_depth_fromVarId, from_depth);
    d3d::clearview(CLEAR_DISCARD_TARGET, 0, 0.f, 0);
    downsampleDepth.render(); // first pass
  }
  ShaderGlobal::set_int(normal_repacking_neededVarId, 0);

  w /= 2;
  h /= 2;

  const int workGroupX = (w + DEPTH_HIERARCHY_DISPATCH_SIZE_X - 1) / DEPTH_HIERARCHY_DISPATCH_SIZE_X;
  const int workGroupY = (h + DEPTH_HIERARCHY_DISPATCH_SIZE_Y - 1) / DEPTH_HIERARCHY_DISPATCH_SIZE_Y;
  ShaderGlobal::set_int(work_group_countVarId, workGroupX * workGroupY);
  ShaderGlobal::set_int4(depth_buffer_sizeVarId, w, h, 0, 0);

  //
  // Downsample far depth
  //
  const int farMipsCount = far_depth->level_count() - 1;
  if (farMipsCount > 0)
  {
    d3d::resource_barrier({far_depth, RB_RO_SRV | RB_STAGE_PIXEL | RB_STAGE_COMPUTE | RB_RO_COPY_SOURCE, 0, 1});
    far_depth->texmiplevel(0, 0);
    d3d::set_tex(STAGE_CS, 0, far_depth);

    ShaderGlobal::set_int(downsampled_depth_mip_countVarId, farMipsCount + 1);
    for (int i = 1; i < far_depth->level_count(); ++i)
    {
      d3d::set_rwtex(STAGE_CS, i - 1, far_depth, 0, i);
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
  if (close_depth && close_depth->level_count() > 1)
  {
    d3d::resource_barrier({close_depth, RB_RO_SRV | RB_STAGE_PIXEL | RB_STAGE_COMPUTE, 0, 1});
    d3d::resource_barrier({atomicCountersBuf.getBuf(), RB_NONE});
    close_depth->texmiplevel(0, 0);
    d3d::set_tex(STAGE_CS, 0, close_depth);

    const int closeMipsCount = close_depth->level_count() - 1;
    for (int i = 1; i < close_depth->level_count(); ++i)
    {
      d3d::set_rwtex(STAGE_CS, i - 1, close_depth, 0, i);
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

  far_depth->texmiplevel(-1, -1);
  if (farMipsCount > 0)
  {
    d3d::resource_barrier( //
      {far_depth, RB_RO_SRV | RB_STAGE_PIXEL | RB_STAGE_COMPUTE | RB_RO_COPY_SOURCE, 1, (unsigned)farMipsCount});
  }
  if (close_depth)
  {
    close_depth->texmiplevel(-1, -1);
    if (!external_barriers && close_depth->level_count() > 1)
    {
      d3d::resource_barrier( //
        {close_depth, RB_RO_SRV | RB_STAGE_PIXEL | RB_STAGE_COMPUTE, 1, (unsigned)close_depth->level_count() - 1u});
    }
  }
  if (far_normals)
    d3d::resource_barrier({far_normals, RB_RO_SRV | RB_STAGE_PIXEL, 0, 1});
  if (motion_vectors)
    d3d::resource_barrier({motion_vectors, RB_RO_SRV | RB_STAGE_PIXEL, 0, 1});
  if (normal_gbuf)
    d3d::resource_barrier({normal_gbuf, RB_RO_SRV | RB_STAGE_PIXEL, 0, 1});
  if (checkerboard_depth)
  {
    checkerboard_depth->texmiplevel(-1, -1);
    if (!external_barriers)
      d3d::resource_barrier({checkerboard_depth, RB_RO_SRV | RB_STAGE_PIXEL | RB_STAGE_VERTEX, 0, 1});
  }
  d3d::resource_barrier({atomicCountersBuf.getBuf(), RB_FLUSH_UAV | RB_STAGE_COMPUTE | RB_SOURCE_STAGE_COMPUTE});

  ShaderGlobal::set_texture(downsample_depth_fromVarId, BAD_TEXTUREID);
}
}; // namespace downsample_depth