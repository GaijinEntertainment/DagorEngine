// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/omm.h>

#include "shaders/omm_texcoord_formats.hlsli"

#include <drv/3d/dag_barrier.h>
#include <drv/3d/dag_buffers.h>
#include <drv/3d/dag_commands.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_info.h>
#include <drv/3d/dag_query.h>
#include <drv/3d/dag_rwResource.h>
#include <drv/3d/dag_sampler.h>
#include <drv/3d/dag_shaderConstants.h>
#include <drv/3d/dag_tex3d.h>
#include <drv/3d/dag_texture.h>

#include <debug/dag_assert.h>
#include <debug/dag_debug.h>
#include <perfMon/dag_statDrv.h>
#include <shaders/dag_computeShaders.h>
#include <shaders/dag_shaders.h>
#include <shaders/dag_shaderVar.h>
#include <shaders/dag_shaderVariableInfo.h>
#include <util/dag_string.h>
#include <util/dag_finally.h>
#include <3d/dag_lockSbuffer.h>

#include <EASTL/utility.h>

#include <cstring>

#ifndef OMM_DEPRECATED_MSG
#define OMM_DEPRECATED_MSG(msg)
#endif
#include <omm.h>
#undef OMM_DEPRECATED_MSG

namespace render::omm
{

// The wire values must agree with the SDK's own enum, since the SDK's three packings are passed through
// as themselves and only the dagor-only ones ride above its range.
static_assert(OMM_TC_UV16_UNORM == ommTexCoordFormat_UV16_UNORM);
static_assert(OMM_TC_UV16_FLOAT == ommTexCoordFormat_UV16_FLOAT);
static_assert(OMM_TC_UV32_FLOAT == ommTexCoordFormat_UV32_FLOAT);
static_assert(OMM_TC_DAGOR_FIRST >= ommTexCoordFormat_MAX_NUM);

uint32_t texcoord_format_to_shader_value(TexCoordFormat format)
{
  switch (format)
  {
    case TexCoordFormat::Float2: return OMM_TC_UV32_FLOAT;
    case TexCoordFormat::Half2: return OMM_TC_UV16_FLOAT;
    case TexCoordFormat::UShort2Norm: return OMM_TC_UV16_UNORM;
    case TexCoordFormat::Short2Fixed4096: return OMM_TC_SHORT2_FIXED4096;
  }
  G_ASSERTF(false, "omm: unhandled TexCoordFormat %u", static_cast<uint32_t>(format));
  return OMM_TC_UV32_FLOAT;
}

namespace
{

static_assert(MAX_TRANSIENT_POOL_BUFFERS == OMM_MAX_TRANSIENT_POOL_BUFFERS);
static_assert(static_cast<uint32_t>(AlphaMode::Test) == static_cast<uint32_t>(ommAlphaMode_Test));
static_assert(static_cast<uint32_t>(IndexFormat::UINT16) == static_cast<uint32_t>(ommIndexFormat_UINT_16));
static_assert(static_cast<uint32_t>(IndexFormat::UINT32) == static_cast<uint32_t>(ommIndexFormat_UINT_32));
static_assert(static_cast<uint32_t>(IndexFormat::UINT8) == static_cast<uint32_t>(ommIndexFormat_UINT_8));
static_assert(static_cast<uint32_t>(OpacityState::UnknownOpaque) == static_cast<uint32_t>(ommOpacityState_UnknownOpaque));
static_assert(static_cast<uint32_t>(Format::OC1_2_State) == static_cast<uint32_t>(ommFormat_OC1_2_State));

struct ResourceView
{
  BaseTexture *texture = nullptr;
  Sbuffer *buffer = nullptr;
};

struct Resources
{
  BaseTexture *alphaTexture = nullptr;
  Sbuffer *texCoordBuffer = nullptr;
  Sbuffer *indexBuffer = nullptr;
  Sbuffer *subdivisionLevelBuffer = nullptr;

  Sbuffer *outOmmArrayData = nullptr;
  Sbuffer *outOmmDescArray = nullptr;
  Sbuffer *outOmmDescArrayHistogram = nullptr;
  Sbuffer *outOmmIndexBuffer = nullptr;
  Sbuffer *outOmmIndexHistogram = nullptr;
  Sbuffer *outPostDispatchInfo = nullptr;
  Sbuffer *transientPoolBuffers[MAX_TRANSIENT_POOL_BUFFERS] = {};
};

struct PostDispatchInfo
{
  uint32_t outOmmArraySizeInBytes = 0;
  uint32_t outOmmDescSizeInBytes = 0;
  uint32_t outStatsTotalOpaqueCount = 0;
  uint32_t outStatsTotalTransparentCount = 0;
  uint32_t outStatsTotalUnknownCount = 0;
  uint32_t outStatsTotalFullyOpaqueCount = 0;
  uint32_t outStatsTotalFullyTransparentCount = 0;
  uint32_t outStatsTotalFullyStatsUnknownCount = 0;
};

struct BufferBarrierState
{
  Sbuffer *buffer = nullptr;
  ResourceBarrier state = RB_NONE;
  bool dirty = false;
};

struct BarrierTracker
{
  BufferBarrierState states[32] = {};
  uint32_t stateCount = 0;

  BufferBarrierState *find(Sbuffer *buffer)
  {
    for (uint32_t i = 0; i < stateCount; ++i)
      if (states[i].buffer == buffer)
        return &states[i];
    return nullptr;
  }

  BufferBarrierState *get(Sbuffer *buffer)
  {
    if (BufferBarrierState *state = find(buffer))
      return state;

    G_ASSERT_RETURN(stateCount < countof(states), nullptr);
    states[stateCount].buffer = buffer;
    states[stateCount].state = RB_NONE;
    states[stateCount].dirty = false;
    return &states[stateCount++];
  }

  void flush(BufferBarrierState &state)
  {
    if (!state.dirty)
      return;

    d3d::resource_barrier({state.buffer, RB_FLUSH_UAV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_COMPUTE});
    state.dirty = false;
  }

  bool transition(Sbuffer *buffer, ResourceBarrier new_state)
  {
    G_ASSERT_RETURN(buffer, false);

    BufferBarrierState *state = get(buffer);
    if (!state)
      return false;

    flush(*state);
    if (state->state != new_state)
    {
      d3d::resource_barrier({buffer, new_state});
      state->state = new_state;
    }
    return true;
  }

  void mark_write(Sbuffer *buffer)
  {
    if (BufferBarrierState *state = get(buffer))
      state->dirty = true;
  }

  void flush_all()
  {
    for (uint32_t i = 0; i < stateCount; ++i)
      flush(states[i]);
  }
};

static uint32_t size_to_dwords(uint32_t byte_size) { return (byte_size + 3u) / 4u; }
static uint32_t size_to_cbuffer_registers(uint32_t byte_size)
{
  return (byte_size + d3d::buffers::CBUFFER_REGISTER_SIZE - 1u) / d3d::buffers::CBUFFER_REGISTER_SIZE;
}

static uint32_t index_format_size(IndexFormat format)
{
  switch (format)
  {
    case IndexFormat::UINT8: return 1;
    case IndexFormat::UINT16: return 2;
    case IndexFormat::UINT32: return 4;
  }

  G_ASSERTF(false, "omm: unsupported index format %u", static_cast<uint32_t>(format));
  return 4;
}

static RaytraceGeometryDescription::IndexFormat to_raytrace_index_format(IndexFormat format)
{
  switch (format)
  {
    case IndexFormat::UINT8: return RaytraceGeometryDescription::IndexFormat::U8;
    case IndexFormat::UINT16: return RaytraceGeometryDescription::IndexFormat::U16;
    case IndexFormat::UINT32: return RaytraceGeometryDescription::IndexFormat::U32;
  }

  G_ASSERTF(false, "omm: unsupported index format %u", static_cast<uint32_t>(format));
  return RaytraceGeometryDescription::IndexFormat::U32;
}

static constexpr ResourceTagType OMM_RESOURCE_TAG = "omm";
static ShaderVariableInfo omm_global_constants_var("omm_global_constants", true);
static ShaderVariableInfo omm_local_constants_var("omm_local_constants", true);
static ShaderVariableInfo omm_sampler_var("omm_sampler0", true);
static ShaderVariableInfo omm_uv_cutout_lines_var("omm_uv_cutout_lines", true);
static ShaderVariableInfo omm_uv_cutout_enabled_var("omm_uv_cutout_enabled", true);
static uint32_t nextConstantBufferNameId = 0;

static bool sdk_ok(Context &ctx, ommResult result, const char *what)
{
  ctx.lastSdkResult = static_cast<int>(result);
  if (result == ommResult_SUCCESS)
    return true;

  logerr("omm: %s failed, sdk result %d", what, static_cast<int>(result));
  return false;
}

static ommTextureAddressMode to_sdk(d3d::AddressMode mode)
{
  switch (mode)
  {
    case d3d::AddressMode::Wrap: return ommTextureAddressMode_Wrap;
    case d3d::AddressMode::Mirror: return ommTextureAddressMode_Mirror;
    case d3d::AddressMode::Clamp: return ommTextureAddressMode_Clamp;
    case d3d::AddressMode::Border: return ommTextureAddressMode_Border;
    case d3d::AddressMode::MirrorOnce: return ommTextureAddressMode_MirrorOnce;
  }

  return ommTextureAddressMode_Wrap;
}

static ommTextureFilterMode to_sdk(d3d::FilterMode mode)
{
  return mode == d3d::FilterMode::Point ? ommTextureFilterMode_Nearest : ommTextureFilterMode_Linear;
}

static ommSamplerDesc to_sdk(const SamplerDesc &desc)
{
  ommSamplerDesc out = ommSamplerDescDefault();
  out.addressingMode = to_sdk(desc.addressingMode);
  out.filter = to_sdk(desc.filter);
  out.borderAlpha = desc.borderAlpha;
  return out;
}

static bool get_render_api(ommGpuRenderAPI &out_api)
{
  const DriverCode driverCode = d3d::get_driver_code();
  if (driverCode.is(d3d::dx12))
  {
    out_api = ommGpuRenderAPI_DX12;
    return true;
  }
  if (driverCode.is(d3d::vulkan))
  {
    out_api = ommGpuRenderAPI_Vulkan;
    return true;
  }

  return false;
}

static ommGpuPipelineConfigDesc make_pipeline_config()
{
  ommGpuPipelineConfigDesc out = ommGpuPipelineConfigDescDefault();
  ommGpuRenderAPI renderApi = ommGpuRenderAPI_DX12;
  if (!get_render_api(renderApi))
    return out;

  out.renderAPI = renderApi;
  return out;
}

static bool choose_alpha_texture_channel(const TextureFormatDesc &desc, uint32_t &out_channel)
{
  if (desc.a.bits > 0)
  {
    out_channel = 3;
    return true;
  }
  if (desc.r.bits > 0)
  {
    out_channel = 0;
    return true;
  }
  if (desc.g.bits > 0)
  {
    out_channel = 1;
    return true;
  }
  if (desc.b.bits > 0)
  {
    out_channel = 2;
    return true;
  }

  return false;
}

static bool fill_alpha_texture_info(const BakeInput &config, ommGpuDispatchConfigDesc &out)
{
  if (!config.alphaTexture)
  {
    logerr("omm: alpha texture is not set");
    return false;
  }

  TextureInfo textureInfo;
  if (!config.alphaTexture->getinfo(textureInfo))
  {
    logerr("omm: failed to query alpha texture info");
    return false;
  }

  uint32_t alphaChannel = 0;
  if (config.alphaTextureChannel <= 3)
    alphaChannel = config.alphaTextureChannel;
  else
  {
    const TextureFormatDesc &formatDesc = get_tex_format_desc(textureInfo.cflg & TEXFMT_MASK);
    if (!choose_alpha_texture_channel(formatDesc, alphaChannel))
    {
      logerr("omm: alpha texture format <%s> has no color or alpha channels", get_tex_format_name(textureInfo.cflg & TEXFMT_MASK));
      return false;
    }
  }

  out.alphaTextureWidth = textureInfo.w;
  out.alphaTextureHeight = textureInfo.h;
  out.alphaTextureChannel = alphaChannel;
  return true;
}

static bool to_sdk(const BakeInput &config, ommGpuDispatchConfigDesc &out)
{
  out = ommGpuDispatchConfigDescDefault();
  out.bakeFlags = static_cast<ommGpuBakeFlags>(config.bakeFlags | COMPUTE_ONLY);
  out.runtimeSamplerDesc = to_sdk(config.runtimeSamplerDesc);
  out.alphaMode = static_cast<ommAlphaMode>(config.alphaMode);
  if (!fill_alpha_texture_info(config, out))
    return false;
  const uint32_t texCoordFormatValue = texcoord_format_to_shader_value(config.texCoordFormat);
  static_assert(sizeof(ommTexCoordFormat) >= 4);
  // Technically UB because C enums have unspecified storage type, but we can't do anything about it...
  out.texCoordFormat = static_cast<ommTexCoordFormat>(texCoordFormatValue); // -V1016
  out.texCoordOffsetInBytes = config.texCoordOffsetInBytes;
  out.texCoordStrideInBytes = config.texCoordStrideInBytes;
  // The SDK derives a stride from its own formats, but cannot for a value outside its enum: GetTexCoord-
  // FormatSize returns 0 and the bake would read all three vertices of every triangle from one offset.
  if (config.texCoordStrideInBytes == 0 && texCoordFormatValue >= OMM_TC_DAGOR_FIRST)
  {
    logerr("omm: texCoordStrideInBytes must be set explicitly for texcoord format %u; the SDK cannot derive it",
      static_cast<uint32_t>(config.texCoordFormat));
    return false;
  }
  out.indexFormat = static_cast<ommIndexFormat>(config.indexFormat);
  out.indexCount = config.indexCount;
  if (!config.indexBuffer || !(config.indexBuffer->getFlags() & SBCF_MISC_ALLOW_RAW))
  {
    logerr("omm: index buffer must support raw shader resource access");
    return false;
  }
  if (!config.texCoordBuffer || !(config.texCoordBuffer->getFlags() & SBCF_MISC_ALLOW_RAW))
  {
    logerr("omm: texcoord buffer must support raw shader resource access");
    return false;
  }
  const uint32_t indexSize = index_format_size(config.indexFormat);
  if (config.indexBufferOffsetInBytes % indexSize != 0)
  {
    logerr("omm: index buffer offset %u is not aligned to index size %u", config.indexBufferOffsetInBytes, indexSize);
    return false;
  }
  out.indexOffset = config.indexBufferOffsetInBytes / indexSize;
  out.indexStrideInBytes = config.indexStrideInBytes;
  out.alphaCutoff = config.alphaCutoff;
  out.alphaCutoffLessEqual = static_cast<ommOpacityState>(config.alphaCutoffLessEqual);
  out.alphaCutoffGreater = static_cast<ommOpacityState>(config.alphaCutoffGreater);
  out.dynamicSubdivisionScale = config.dynamicSubdivisionScale;
  out.globalFormat = static_cast<ommFormat>(config.globalFormat);
  out.maxSubdivisionLevel = config.maxSubdivisionLevel;
  out.enableSubdivisionLevelBuffer = config.enableSubdivisionLevelBuffer ? 1 : 0;
  out.maxOutOmmArraySize = config.maxOutOmmArraySize;
  out.maxScratchMemorySize = static_cast<ommGpuScratchMemoryBudget>(config.maxScratchMemorySize);
  return true;
}

static d3d::AddressMode to_dagor_address_mode(ommTextureAddressMode mode)
{
  switch (mode)
  {
    case ommTextureAddressMode_Wrap: return d3d::AddressMode::Wrap;
    case ommTextureAddressMode_Mirror: return d3d::AddressMode::Mirror;
    case ommTextureAddressMode_Clamp: return d3d::AddressMode::Clamp;
    case ommTextureAddressMode_Border: return d3d::AddressMode::Border;
    case ommTextureAddressMode_MirrorOnce: return d3d::AddressMode::MirrorOnce;
    default: return d3d::AddressMode::Wrap;
  }
}

static d3d::FilterMode to_dagor_filter_mode(ommTextureFilterMode mode)
{
  return mode == ommTextureFilterMode_Nearest ? d3d::FilterMode::Point : d3d::FilterMode::Linear;
}

static d3d::MipMapMode to_dagor_mip_mode(ommTextureFilterMode mode)
{
  return mode == ommTextureFilterMode_Nearest ? d3d::MipMapMode::Point : d3d::MipMapMode::Linear;
}

static d3d::SamplerHandle request_sampler(const ommSamplerDesc &desc)
{
  const d3d::AddressMode addressMode = to_dagor_address_mode(desc.addressingMode);
  const d3d::SamplerInfo sampler{
    .mip_map_mode = to_dagor_mip_mode(desc.filter),
    .filter_mode = to_dagor_filter_mode(desc.filter),
    .address_mode_u = addressMode,
    .address_mode_v = addressMode,
    .address_mode_w = addressMode,
    .border_color =
      d3d::BorderColor(desc.borderAlpha > 0.5f ? d3d::BorderColor::Color::OpaqueWhite : d3d::BorderColor::Color::TransparentBlack),
  };
  return d3d::request_sampler(sampler);
}

static bool set_pipeline_info(Context &ctx)
{
  const ommGpuPipelineInfoDesc *info = nullptr;
  if (!sdk_ok(ctx, ommGpuGetPipelineDesc(static_cast<ommGpuPipeline>(ctx.pipeline), &info), "Gpu::GetPipelineDesc"))
    return false;

  ctx.pipelineInfo = info;
  if (!info)
    return false;

  ctx.staticSamplerCount = 0;
  for (uint32_t i = 0; i < info->staticSamplersNum; ++i)
  {
    if (i >= MAX_STATIC_SAMPLERS)
    {
      logerr("omm: SDK requested %u static samplers, wrapper supports %u", info->staticSamplersNum, MAX_STATIC_SAMPLERS);
      return false;
    }

    ctx.staticSamplers[i] = request_sampler(info->staticSamplers[i].desc);
    ctx.staticSamplerRegisters[i] = info->staticSamplers[i].registerIndex;
    ctx.staticSamplerCount++;
  }

  return true;
}

struct ShaderNameMap
{
  const char *sdkName = nullptr;
  const char *dagorName = nullptr;
};

static const ShaderNameMap COMPUTE_SHADER_NAMES[] = {
  {"omm_clear_buffer.cs", "omm_clear_buffer_cs"},
  {"omm_init_buffers_cs.cs", "omm_init_buffers_cs_cs"},
  {"omm_init_buffers_gfx.cs", "omm_init_buffers_gfx_cs"},
  {"omm_work_setup_bake_only_cs.cs", "omm_work_setup_bake_only_cs_cs"},
  {"omm_work_setup_cs.cs", "omm_work_setup_cs_cs"},
  {"omm_work_setup_gfx.cs", "omm_work_setup_gfx_cs"},
  {"omm_work_setup_bake_only_gfx.cs", "omm_work_setup_bake_only_gfx_cs"},
  {"omm_post_build_info.cs", "omm_post_build_info_cs"},
  {"omm_rasterize_cs_r.cs", "omm_rasterize_cs_r_cs"},
  {"omm_rasterize_cs_g.cs", "omm_rasterize_cs_g_cs"},
  {"omm_rasterize_cs_b.cs", "omm_rasterize_cs_b_cs"},
  {"omm_rasterize_cs_a.cs", "omm_rasterize_cs_a_cs"},
  {"omm_compress.cs", "omm_compress_cs"},
  {"omm_desc_patch.cs", "omm_desc_patch_cs"},
  {"omm_index_write.cs", "omm_index_write_cs"},
};

struct ResourceNameMap
{
  const char *sdkName = nullptr;
  const char *shaderVars[8] = {};
  uint32_t shaderVarCount = 0;
};

#define OMM_VAR(name) "omm_" #name

static const ResourceNameMap COMPUTE_RESOURCE_NAMES[] = {
  {"omm_clear_buffer.cs", {OMM_VAR(u_targetBuffer)}, 1},
  {"omm_init_buffers_cs.cs", {OMM_VAR(u_heap0), OMM_VAR(u_ommDescArrayHistogramBuffer), OMM_VAR(u_ommIndexHistogramBuffer)}, 3},
  {"omm_init_buffers_gfx.cs", {OMM_VAR(u_buffer0), OMM_VAR(u_ommDescArrayHistogramBuffer), OMM_VAR(u_ommIndexHistogramBuffer)}, 3},
  {"omm_work_setup_bake_only_cs.cs", {OMM_VAR(t_ommDescArrayBuffer), OMM_VAR(t_ommIndexBuffer), OMM_VAR(u_heap0), OMM_VAR(u_heap1)},
    4},
  {"omm_work_setup_cs.cs",
    {OMM_VAR(t_indexBuffer), OMM_VAR(t_texCoordBuffer), OMM_VAR(u_ommDescArrayBuffer), OMM_VAR(u_ommDescArrayHistogramBuffer),
      OMM_VAR(u_heap0), OMM_VAR(u_heap1)},
    6},
  {"omm_work_setup_gfx.cs",
    {OMM_VAR(t_indexBuffer), OMM_VAR(t_texCoordBuffer), OMM_VAR(u_ommDescArrayBuffer), OMM_VAR(u_ommDescArrayHistogramBuffer),
      OMM_VAR(u_heap0), OMM_VAR(u_heap1)},
    6},
  {"omm_work_setup_bake_only_gfx.cs", {OMM_VAR(t_ommDescArrayBuffer), OMM_VAR(t_ommIndexBuffer), OMM_VAR(u_heap0), OMM_VAR(u_heap1)},
    4},
  {"omm_post_build_info.cs", {OMM_VAR(u_heap0_read), OMM_VAR(u_postBuildInfo)}, 2},
  {"omm_rasterize_cs_r.cs",
    {OMM_VAR(t_alphaTexture), OMM_VAR(t_indexBuffer), OMM_VAR(t_texCoordBuffer), OMM_VAR(t_heap0), OMM_VAR(t_heap1), OMM_VAR(u_heap0),
      OMM_VAR(u_vmArrayBuffer)},
    7},
  {"omm_rasterize_cs_g.cs",
    {OMM_VAR(t_alphaTexture), OMM_VAR(t_indexBuffer), OMM_VAR(t_texCoordBuffer), OMM_VAR(t_heap0), OMM_VAR(t_heap1), OMM_VAR(u_heap0),
      OMM_VAR(u_vmArrayBuffer)},
    7},
  {"omm_rasterize_cs_b.cs",
    {OMM_VAR(t_alphaTexture), OMM_VAR(t_indexBuffer), OMM_VAR(t_texCoordBuffer), OMM_VAR(t_heap0), OMM_VAR(t_heap1), OMM_VAR(u_heap0),
      OMM_VAR(u_vmArrayBuffer)},
    7},
  {"omm_rasterize_cs_a.cs",
    {OMM_VAR(t_alphaTexture), OMM_VAR(t_indexBuffer), OMM_VAR(t_texCoordBuffer), OMM_VAR(t_heap0), OMM_VAR(t_heap1), OMM_VAR(u_heap0),
      OMM_VAR(u_vmArrayBuffer)},
    7},
  {"omm_compress.cs", {OMM_VAR(t_heap0), OMM_VAR(u_vmArrayBuffer), OMM_VAR(u_heap1)}, 3},
  {"omm_desc_patch.cs",
    {OMM_VAR(t_heap1), OMM_VAR(t_ommDescArrayBuffer), OMM_VAR(u_ommIndexHistogramBuffer), OMM_VAR(u_postBuildInfo), OMM_VAR(u_heap0)},
    5},
  {"omm_index_write.cs", {OMM_VAR(t_heap0), OMM_VAR(u_ommIndexBuffer)}, 2},
};

#undef OMM_VAR

static const char *find_dagor_shader_name(const ShaderNameMap *names, uint32_t name_count, const char *sdk_name)
{
  if (!sdk_name)
    return nullptr;

  for (uint32_t i = 0; i < name_count; ++i)
    if (strcmp(names[i].sdkName, sdk_name) == 0)
      return names[i].dagorName;

  return nullptr;
}

static const ResourceNameMap *find_resource_names(const char *sdk_name)
{
  if (!sdk_name)
    return nullptr;

  for (uint32_t i = 0; i < countof(COMPUTE_RESOURCE_NAMES); ++i)
    if (strcmp(COMPUTE_RESOURCE_NAMES[i].sdkName, sdk_name) == 0)
      return &COMPUTE_RESOURCE_NAMES[i];

  return nullptr;
}

static bool load_compute_shader(Context &ctx, uint32_t pipeline_index, const ommGpuPipelineDesc &pipeline)
{
  const char *shaderName =
    find_dagor_shader_name(COMPUTE_SHADER_NAMES, countof(COMPUTE_SHADER_NAMES), pipeline.compute.shaderFileName);
  if (!shaderName)
  {
    logerr("omm: unsupported SDK compute shader <%s>", pipeline.compute.shaderFileName ? pipeline.compute.shaderFileName : "");
    return false;
  }

  ctx.computeShaders[pipeline_index] = ComputeShader(shaderName, true);
  if (!ctx.computeShaders[pipeline_index])
  {
    logerr("omm: failed to load compute shader <%s>", shaderName);
    return false;
  }

  if (ctx.computeShaders[pipeline_index].getComputeProgram() == BAD_PROGRAM)
  {
    logerr("omm: compute shader <%s> has no valid program", shaderName);
    return false;
  }

  return true;
}

static const char *get_compute_shader_name(const ommGpuPipelineDesc &pipeline)
{
  if (pipeline.type != ommGpuPipelineType_Compute)
    return "";
  return find_dagor_shader_name(COMPUTE_SHADER_NAMES, countof(COMPUTE_SHADER_NAMES), pipeline.compute.shaderFileName);
}

static bool load_pipeline_shaders(Context &ctx)
{
  const auto *info = static_cast<const ommGpuPipelineInfoDesc *>(ctx.pipelineInfo);
  if (!info)
    return false;

  if (info->pipelineNum > MAX_PIPELINES)
  {
    logerr("omm: SDK requested %u pipelines, wrapper supports %u", info->pipelineNum, MAX_PIPELINES);
    return false;
  }

  for (uint32_t i = 0; i < MAX_PIPELINES; ++i)
    ctx.computeShaders[i] = ComputeShader();

  ctx.programCount = info->pipelineNum;
  for (uint32_t i = 0; i < info->pipelineNum; ++i)
  {
    const ommGpuPipelineDesc &pipeline = info->pipelines[i];
    if (pipeline.type == ommGpuPipelineType_Compute)
    {
      if (!load_compute_shader(ctx, i, pipeline))
        return false;
    }
    else if (pipeline.type == ommGpuPipelineType_Graphics)
    {
      continue;
    }
    else
      return false;
  }

  return true;
}

static ResourceView resolve_resource(Context &ctx, const Resources &resources, const ommGpuResource &resource)
{
  switch (resource.type)
  {
    case ommGpuResourceType_IN_ALPHA_TEXTURE: return {.texture = resources.alphaTexture};
    case ommGpuResourceType_IN_TEXCOORD_BUFFER: return {.buffer = resources.texCoordBuffer};
    case ommGpuResourceType_IN_INDEX_BUFFER: return {.buffer = resources.indexBuffer};
    case ommGpuResourceType_IN_SUBDIVISION_LEVEL_BUFFER: return {.buffer = resources.subdivisionLevelBuffer};
    case ommGpuResourceType_OUT_OMM_ARRAY_DATA: return {.buffer = resources.outOmmArrayData};
    case ommGpuResourceType_OUT_OMM_DESC_ARRAY: return {.buffer = resources.outOmmDescArray};
    case ommGpuResourceType_OUT_OMM_DESC_ARRAY_HISTOGRAM: return {.buffer = resources.outOmmDescArrayHistogram};
    case ommGpuResourceType_OUT_OMM_INDEX_BUFFER: return {.buffer = resources.outOmmIndexBuffer};
    case ommGpuResourceType_OUT_OMM_INDEX_HISTOGRAM: return {.buffer = resources.outOmmIndexHistogram};
    case ommGpuResourceType_OUT_POST_DISPATCH_INFO: return {.buffer = resources.outPostDispatchInfo};
    case ommGpuResourceType_TRANSIENT_POOL_BUFFER:
      return resource.indexInPool < MAX_TRANSIENT_POOL_BUFFERS
               ? ResourceView{.buffer = resources.transientPoolBuffers[resource.indexInPool]}
               : ResourceView{};
    default: return {};
  }
}

static bool transition_resource(Context &ctx, BarrierTracker &barriers, const Resources &resources, const ommGpuResource &resource)
{
  const ResourceView view = resolve_resource(ctx, resources, resource);

  if (resource.stateNeeded == ommGpuDescriptorType_TextureRead)
  {
    if (!view.texture)
      return false;

    return true;
  }

  if (!view.buffer)
    return false;

  const ResourceBarrier state =
    resource.stateNeeded == ommGpuDescriptorType_RawBufferWrite ? (RB_RW_UAV | RB_STAGE_COMPUTE) : (RB_RO_SRV | RB_STAGE_COMPUTE);
  return barriers.transition(view.buffer, state);
}

static void mark_written_resource(Context &ctx, BarrierTracker &barriers, const Resources &resources, const ommGpuResource &resource)
{
  if (resource.stateNeeded != ommGpuDescriptorType_RawBufferWrite)
    return;

  const ResourceView view = resolve_resource(ctx, resources, resource);
  if (view.buffer)
    barriers.mark_write(view.buffer);
}

static bool bind_resource_var(const char *shader_var, const ResourceView &view, ommGpuDescriptorType type)
{
  const int varId = get_shader_variable_id(shader_var, true);
  switch (type)
  {
    case ommGpuDescriptorType_TextureRead:
      if (!view.texture)
        return false;
      return ShaderGlobal::set_texture_unsafe(varId, view.texture);
    case ommGpuDescriptorType_BufferRead:
    case ommGpuDescriptorType_RawBufferRead:
    case ommGpuDescriptorType_RawBufferWrite:
      if (!view.buffer)
        return false;
      return ShaderGlobal::set_buffer_unsafe(varId, view.buffer);
    default: break;
  }

  return false;
}

static void unbind_resource_var(const char *shader_var, ommGpuDescriptorType type)
{
  const int varId = get_shader_variable_id(shader_var, true);
  switch (type)
  {
    case ommGpuDescriptorType_TextureRead: ShaderGlobal::set_texture_unsafe(varId, nullptr); break;
    case ommGpuDescriptorType_BufferRead:
    case ommGpuDescriptorType_RawBufferRead:
    case ommGpuDescriptorType_RawBufferWrite: ShaderGlobal::set_buffer_unsafe(varId, nullptr); break;
    default: break;
  }
}

static bool bind_sampler(d3d::SamplerHandle sampler) { return omm_sampler_var.set_sampler(sampler); }

static void unbind_sampler() { omm_sampler_var.set_sampler(d3d::INVALID_SAMPLER_HANDLE); }

static bool bind_resources(Context &ctx, BarrierTracker &barriers, const Resources &resources, const ommGpuPipelineDesc &pipeline,
  const ommGpuResource *resource_list, uint32_t resource_count)
{
  const ResourceNameMap *resourceNames = find_resource_names(pipeline.compute.shaderFileName);
  if (!resourceNames)
  {
    logerr("omm: missing resource name map for SDK shader <%s>",
      pipeline.compute.shaderFileName ? pipeline.compute.shaderFileName : "");
    return false;
  }
  if (resourceNames->shaderVarCount != resource_count)
  {
    logerr("omm: SDK shader <%s> expected %u resources, got %u",
      pipeline.compute.shaderFileName ? pipeline.compute.shaderFileName : "", resourceNames->shaderVarCount, resource_count);
    return false;
  }

  const ommGpuDescriptorRangeDesc *ranges = pipeline.compute.descriptorRanges;
  const uint32_t rangeCount = pipeline.compute.descriptorRangeNum;

  uint32_t resourceIndex = 0;
  for (uint32_t rangeIndex = 0; rangeIndex < rangeCount; ++rangeIndex)
  {
    const ommGpuDescriptorRangeDesc &range = ranges[rangeIndex];
    for (uint32_t i = 0; i < range.descriptorNum; ++i)
    {
      if (resourceIndex >= resource_count)
        return false;

      const ommGpuResource &resource = resource_list[resourceIndex++];
      if (!transition_resource(ctx, barriers, resources, resource))
        return false;

      const ResourceView view = resolve_resource(ctx, resources, resource);
      if (!bind_resource_var(resourceNames->shaderVars[resourceIndex - 1], view, range.descriptorType))
        return false;
    }
  }

  return resourceIndex == resource_count;
}

static void unbind_resources(const ommGpuPipelineDesc &pipeline)
{
  const ResourceNameMap *resourceNames = find_resource_names(pipeline.compute.shaderFileName);
  if (!resourceNames)
    return;

  const ommGpuDescriptorRangeDesc *ranges = pipeline.compute.descriptorRanges;
  const uint32_t rangeCount = pipeline.compute.descriptorRangeNum;

  uint32_t resourceIndex = 0;
  for (uint32_t rangeIndex = 0; rangeIndex < rangeCount; ++rangeIndex)
  {
    const ommGpuDescriptorRangeDesc &range = ranges[rangeIndex];
    for (uint32_t i = 0; i < range.descriptorNum; ++i)
    {
      if (resourceIndex >= resourceNames->shaderVarCount)
        return;

      unbind_resource_var(resourceNames->shaderVars[resourceIndex++], range.descriptorType);
    }
  }
}

static void mark_written_resources(Context &ctx, BarrierTracker &barriers, const Resources &resources,
  const ommGpuResource *resource_list, uint32_t resource_count)
{
  for (uint32_t i = 0; i < resource_count; ++i)
    mark_written_resource(ctx, barriers, resources, resource_list[i]);
}

static bool create_constant_buffer(dag::Vector<UniqueBuf> &buffers, const uint8_t *data, uint32_t byte_size, const char *name,
  ResourceTagType tag, D3DRESID &out_buffer_id)
{
  out_buffer_id = BAD_D3DRESID;
  if (!data || byte_size == 0)
    return true;

  const String uniqueName(0, "%s_%u", name, nextConstantBufferNameId++);
  UniqueBuf buffer = dag::buffers::create_one_frame_cb(size_to_cbuffer_registers(byte_size), uniqueName.c_str(), tag);
  if (!buffer)
  {
    logerr("omm: failed to create constant buffer <%s> of size %u", name, byte_size);
    return false;
  }

  if (!buffer.getBuf()->updateData(0, byte_size, data, VBLOCK_WRITEONLY | VBLOCK_DISCARD))
  {
    logerr("omm: failed to update constant buffer <%s>", name);
    return false;
  }

  out_buffer_id = buffer.getBufId();
  buffers.push_back(eastl::move(buffer));
  return true;
}

static bool bind_constants(PendingBake &bake, const ommGpuDispatchChain &chain, const uint8_t *local_data, uint32_t local_size)
{
  D3DRESID globalBufferId = BAD_D3DRESID;
  D3DRESID localBufferId = BAD_D3DRESID;
  if (!create_constant_buffer(bake.constantBuffers, chain.globalCBufferData, chain.globalCBufferDataSize, "omm_global_constants",
        OMM_RESOURCE_TAG, globalBufferId) ||
      !create_constant_buffer(bake.constantBuffers, local_data, local_size, "omm_local_constants", OMM_RESOURCE_TAG, localBufferId))
    return false;

  return omm_global_constants_var.set_buffer(globalBufferId) && omm_local_constants_var.set_buffer(localBufferId);
}

static void unbind_constants()
{
  omm_global_constants_var.set_buffer(BAD_D3DRESID);
  omm_local_constants_var.set_buffer(BAD_D3DRESID);
}

static bool validate_program(const Context &ctx, uint32_t pipeline_index)
{
  return pipeline_index < ctx.programCount && ctx.computeShaders[pipeline_index] &&
         ctx.computeShaders[pipeline_index].getComputeProgram() != BAD_PROGRAM;
}

static bool execute_compute(Context &ctx, PendingBake &bake, BarrierTracker &barriers, const Resources &resources,
  const ommGpuDispatchChain &chain, const ommGpuComputeDesc &desc, d3d::SamplerHandle sampler)
{
  TIME_D3D_PROFILE_NAME(omm_compute_dispatch, desc.name ? desc.name : "omm_compute_dispatch");

  const auto *info = static_cast<const ommGpuPipelineInfoDesc *>(ctx.pipelineInfo);
  if (!info || desc.pipelineIndex >= info->pipelineNum || !validate_program(ctx, desc.pipelineIndex))
    return false;

  const ommGpuPipelineDesc &pipeline = info->pipelines[desc.pipelineIndex];
  if (pipeline.type != ommGpuPipelineType_Compute)
    return false;

  FINALLY([&] { unbind_sampler(); });
  FINALLY([&] { unbind_constants(); });
  FINALLY([&] { unbind_resources(pipeline); });

  if (!bind_sampler(sampler) || !bind_constants(bake, chain, desc.localConstantBufferData, desc.localConstantBufferDataSize) ||
      !bind_resources(ctx, barriers, resources, pipeline, desc.resources, desc.resourceNum))
    return false;

  if (!ctx.computeShaders[desc.pipelineIndex].dispatchGroups(desc.gridWidth, desc.gridHeight, 1))
  {
    const char *shaderName = get_compute_shader_name(pipeline);
    logerr("omm: compute dispatch <%s> failed for shader <%s>", desc.name ? desc.name : "", shaderName ? shaderName : "");
    return false;
  }

  mark_written_resources(ctx, barriers, resources, desc.resources, desc.resourceNum);
  return true;
}

static bool execute_compute_indirect(Context &ctx, PendingBake &bake, BarrierTracker &barriers, const Resources &resources,
  const ommGpuDispatchChain &chain, const ommGpuComputeIndirectDesc &desc, d3d::SamplerHandle sampler)
{
  TIME_D3D_PROFILE_NAME(omm_compute_indirect_dispatch, desc.name ? desc.name : "omm_compute_indirect_dispatch");

  const auto *info = static_cast<const ommGpuPipelineInfoDesc *>(ctx.pipelineInfo);
  if (!info || desc.pipelineIndex >= info->pipelineNum || !validate_program(ctx, desc.pipelineIndex))
    return false;

  const ommGpuPipelineDesc &pipeline = info->pipelines[desc.pipelineIndex];
  if (pipeline.type != ommGpuPipelineType_Compute)
    return false;

  const ResourceView indirect = resolve_resource(ctx, resources, desc.indirectArg);
  if (!indirect.buffer)
    return false;

  FINALLY([&] { unbind_sampler(); });
  FINALLY([&] { unbind_constants(); });
  FINALLY([&] { unbind_resources(pipeline); });

  if (!bind_sampler(sampler) || !bind_constants(bake, chain, desc.localConstantBufferData, desc.localConstantBufferDataSize) ||
      !bind_resources(ctx, barriers, resources, pipeline, desc.resources, desc.resourceNum))
    return false;

  if (!barriers.transition(indirect.buffer, RB_RO_INDIRECT_BUFFER))
    return false;
  if (!ctx.computeShaders[desc.pipelineIndex].dispatchIndirect(indirect.buffer, static_cast<int>(desc.indirectArgByteOffset)))
  {
    const char *shaderName = get_compute_shader_name(pipeline);
    logerr("omm: indirect compute dispatch <%s> failed for shader <%s>", desc.name ? desc.name : "", shaderName ? shaderName : "");
    return false;
  }

  mark_written_resources(ctx, barriers, resources, desc.resources, desc.resourceNum);
  return true;
}

static bool create_buffer(UniqueBuf &buffer, uint32_t byte_size, unsigned flags, const char *name, ResourceTagType tag)
{
  if (byte_size == 0)
  {
    buffer.close();
    return true;
  }

  buffer = dag::create_sbuffer(4, static_cast<int>(size_to_dwords(byte_size)), flags, 0, name, tag);
  if (!buffer)
  {
    logerr("omm: failed to create buffer <%s> of size %u", name, byte_size);
    return false;
  }

  return true;
}

static bool create_readback_buffer(UniqueBuf &buffer, uint32_t byte_size, const char *name, ResourceTagType tag)
{
  if (byte_size == 0)
  {
    buffer.close();
    return true;
  }

  buffer = dag::buffers::create_ua_byte_address_readback(size_to_dwords(byte_size), name, d3d::buffers::Init::No, tag);
  if (!buffer)
  {
    logerr("omm: failed to create readback buffer <%s> of size %u", name, byte_size);
    return false;
  }

  return true;
}

static void close_pending_bake(PendingBake &bake)
{
  const uint32_t generation = bake.generation;
  bake = {};
  bake.generation = generation;
}

static PendingBake *get_pending_bake(Context &ctx, BakeHandle handle)
{
  if (handle.slot >= MAX_PENDING_BAKES)
    return nullptr;

  PendingBake &bake = ctx.pendingBakes[handle.slot];
  if (bake.state == PendingBakeState::Free || bake.generation != handle.generation)
    return nullptr;

  return &bake;
}

static Resources make_resources(const BakeInput &input, const PendingBake &bake)
{
  Resources resources{
    .alphaTexture = input.alphaTexture,
    .texCoordBuffer = input.texCoordBuffer,
    .indexBuffer = input.indexBuffer,
    .subdivisionLevelBuffer = input.subdivisionLevelBuffer,
    .outOmmArrayData = bake.outOmmArrayData.getBuf(),
    .outOmmDescArray = bake.outOmmDescArray.getBuf(),
    .outOmmDescArrayHistogram = bake.outOmmDescArrayHistogram.getBuf(),
    .outOmmIndexBuffer = bake.outOmmIndexBuffer.getBuf(),
    .outOmmIndexHistogram = bake.outOmmIndexHistogram.getBuf(),
    .outPostDispatchInfo = bake.outPostDispatchInfo.getBuf(),
  };
  for (uint32_t i = 0; i < MAX_TRANSIENT_POOL_BUFFERS; ++i)
    resources.transientPoolBuffers[i] = bake.transientPoolBuffers[i].getBuf();
  return resources;
}

static bool clear_omm_array_data(PendingBake &bake)
{
  Sbuffer *buffer = bake.outOmmArrayData.getBuf();
  if (!buffer)
    return true;

  TIME_D3D_PROFILE(omm_clear_array_data);

  return d3d::zero_rwbufi(buffer);
}

static bool copy_for_readback(Sbuffer *src, Sbuffer *dst)
{
  G_ASSERT_RETURN(src && dst, false);

  if (!src->copyTo(dst))
    return false;

  if (dst->lock(0, 0, static_cast<void **>(nullptr), VBLOCK_READONLY))
    dst->unlock();

  return true;
}

static bool issue_readbacks(PendingBake &bake)
{
  // Histograms are optional statistics: allocate_pending_bake leaves both the device and readback
  // buffers null when the SDK reports a zero-sized histogram, so skip those copies. Post-dispatch
  // info is always present.
  const bool haveArrayHistogram = bake.outOmmDescArrayHistogram && bake.readbackOmmDescArrayHistogram;
  const bool haveIndexHistogram = bake.outOmmIndexHistogram && bake.readbackOmmIndexHistogram;

  if (haveArrayHistogram && !copy_for_readback(bake.outOmmDescArrayHistogram.getBuf(), bake.readbackOmmDescArrayHistogram.getBuf()))
    return false;
  if (haveIndexHistogram && !copy_for_readback(bake.outOmmIndexHistogram.getBuf(), bake.readbackOmmIndexHistogram.getBuf()))
    return false;
  if (!copy_for_readback(bake.outPostDispatchInfo.getBuf(), bake.readbackPostDispatchInfo.getBuf()))
    return false;

  if (!bake.readbackQuery)
    bake.readbackQuery.reset(d3d::create_event_query());
  return bake.readbackQuery && d3d::issue_event_query(bake.readbackQuery.get());
}

static bool convert_histogram(Sbuffer *readback, uint32_t byte_size, dag::Vector<raytrace::OpacityMicroMapDescription> &out_descs)
{
  out_descs.clear();
  if (!readback || byte_size == 0)
    return true;

  struct HistogramEntry
  {
    uint32_t count;
    uint16_t subdivisionLevel;
    uint16_t format;
  };
  static_assert(sizeof(HistogramEntry) == 8);

  G_ASSERTF(byte_size % sizeof(HistogramEntry) == 0, "omm: histogram byte size %u is not a multiple of entry size %u", byte_size,
    uint32_t(sizeof(HistogramEntry)));
  const uint32_t count = byte_size / sizeof(HistogramEntry);

  auto entries = lock_sbuffer<const HistogramEntry>(readback, 0, count, VBLOCK_READONLY);
  if (!entries)
    return false;

  out_descs.reserve(count);
  for (uint32_t i = 0; i < count; ++i)
  {
    if (entries[i].count == 0)
      continue;

    raytrace::OpacityMicroMapDescription desc;
    desc.count = entries[i].count;
    desc.subdivisionLevel = entries[i].subdivisionLevel;
    desc.format = static_cast<raytrace::OpacityMicroMapFormat>(entries[i].format);
    out_descs.push_back(desc);
  }

  return true;
}

static bool read_post_dispatch_info(Sbuffer *readback, PostDispatchInfo &out_info)
{
  out_info = {};
  if (!readback)
    return true;

  auto locked = lock_sbuffer<const ommGpuPostDispatchInfo>(readback, 0, 1, VBLOCK_READONLY);
  if (!locked)
    return false;

  const ommGpuPostDispatchInfo *sdkInfo = locked.get();
  out_info.outOmmArraySizeInBytes = sdkInfo->outOmmArraySizeInBytes;
  out_info.outOmmDescSizeInBytes = sdkInfo->outOmmDescSizeInBytes;
  out_info.outStatsTotalOpaqueCount = sdkInfo->outStatsTotalOpaqueCount;
  out_info.outStatsTotalTransparentCount = sdkInfo->outStatsTotalTransparentCount;
  out_info.outStatsTotalUnknownCount = sdkInfo->outStatsTotalUnknownCount;
  out_info.outStatsTotalFullyOpaqueCount = sdkInfo->outStatsTotalFullyOpaqueCount;
  out_info.outStatsTotalFullyTransparentCount = sdkInfo->outStatsTotalFullyTransparentCount;
  out_info.outStatsTotalFullyStatsUnknownCount = sdkInfo->outStatsTotalFullyStatsUnknownCount;

  return true;
}

static BakeStats make_bake_stats(const PostDispatchInfo &info)
{
  return {
    .totalOpaqueCount = info.outStatsTotalOpaqueCount,
    .totalTransparentCount = info.outStatsTotalTransparentCount,
    .totalUnknownCount = info.outStatsTotalUnknownCount,
    .totalFullyOpaqueCount = info.outStatsTotalFullyOpaqueCount,
    .totalFullyTransparentCount = info.outStatsTotalFullyTransparentCount,
    .totalFullyUnknownCount = info.outStatsTotalFullyStatsUnknownCount,
  };
}

} // namespace

// For the debug viewer: its alpha overlay must sample with the same sampler as the bake. Outside the
// unnamed namespace above, thus the viewer's extern declaration links to it.
d3d::SamplerHandle request_runtime_sampler(const SamplerDesc &desc) { return request_sampler(to_sdk(desc)); }

static bool validate_shader_vars()
{
  bool allPresent = true;
  auto require = [&allPresent](const char *name) {
    if (!VariableMap::isVariablePresent(get_shader_variable_id(name, true)))
    {
      logerr("omm: required shader variable <%s> is missing", name);
      allPresent = false;
    }
  };

  require("omm_global_constants");
  require("omm_local_constants");
  require("omm_sampler0");
  for (const ResourceNameMap &names : COMPUTE_RESOURCE_NAMES)
    for (uint32_t i = 0; i < names.shaderVarCount; ++i)
      require(names.shaderVars[i]);

  return allPresent;
}

bool init(Context &ctx)
{
  shutdown(ctx);

  ommGpuRenderAPI renderApi = ommGpuRenderAPI_DX12;
  if (!get_render_api(renderApi))
  {
    logerr("omm: unsupported d3d driver <%s>", d3d::get_driver_name());
    return false;
  }

  ommBakerCreationDesc bakerDesc = ommBakerCreationDescDefault();
  bakerDesc.type = ommBakerType_GPU;
  bakerDesc.messageInterface.messageCallback = [](ommMessageSeverity severity, const char *message, void *userArg) {
    switch (severity)
    {
      case ommMessageSeverity_Info: logdbg("omm: info: %s", message); break;
      case ommMessageSeverity_PerfWarning: logdbg("omm: perf: %s", message); break;
      case ommMessageSeverity_Error: logdbg("omm: error: %s", message); break;
      case ommMessageSeverity_Fatal: logdbg("omm: fatal: %s", message); break;
    }
  };
  ommBaker baker = nullptr;
  if (!sdk_ok(ctx, ommCreateBaker(&bakerDesc, &baker), "CreateBaker"))
    return false;
  ctx.baker = baker;

  ommGpuPipelineConfigDesc pipelineDesc = make_pipeline_config();
  ommGpuPipeline pipeline = nullptr;
  if (!sdk_ok(ctx, ommGpuCreatePipeline(baker, &pipelineDesc, &pipeline), "Gpu::CreatePipeline"))
  {
    shutdown(ctx);
    return false;
  }
  ctx.pipeline = pipeline;

  if (!set_pipeline_info(ctx) || !load_pipeline_shaders(ctx) || !validate_shader_vars())
  {
    shutdown(ctx);
    return false;
  }

  return true;
}

void shutdown(Context &ctx)
{
  debug_shutdown();

  if (ctx.pipeline && ctx.baker)
    ommGpuDestroyPipeline(static_cast<ommBaker>(ctx.baker), static_cast<ommGpuPipeline>(ctx.pipeline));
  if (ctx.baker)
    ommDestroyBaker(static_cast<ommBaker>(ctx.baker));

  ctx = {};
}

// Sanity cap on the VRAM a single bake may allocate. The SDK-reported pre-dispatch sizes drive the
// allocations directly, so a malformed/oversized response is rejected here rather than allowed to
// consume pending-bake VRAM until an allocation fails. Tunable; well above any legitimate single-mesh
// bake.
static constexpr uint64_t OMM_MAX_BAKE_ALLOCATION_BYTES = 256ull << 20;

static bool fill_pre_dispatch_info(Context &ctx, const BakeInput &input, PendingBake &out_info)
{
  if (!ctx.pipeline)
    return false;

  ommGpuDispatchConfigDesc sdkConfig;
  if (!to_sdk(input, sdkConfig))
    return false;

  ommGpuPreDispatchInfo sdkInfo = ommGpuPreDispatchInfoDefault();
  if (
    !sdk_ok(ctx, ommGpuGetPreDispatchInfo(static_cast<ommGpuPipeline>(ctx.pipeline), &sdkConfig, &sdkInfo), "Gpu::GetPreDispatchInfo"))
    return false;

  out_info.outOmmIndexBufferFormat = static_cast<IndexFormat>(sdkInfo.outOmmIndexBufferFormat);
  out_info.outOmmIndexCount = sdkInfo.outOmmIndexCount;
  out_info.outOmmArraySizeInBytes = sdkInfo.outOmmArraySizeInBytes;
  out_info.outOmmDescSizeInBytes = sdkInfo.outOmmDescSizeInBytes;
  out_info.outOmmIndexBufferSizeInBytes = sdkInfo.outOmmIndexBufferSizeInBytes;
  out_info.outOmmArrayHistogramSizeInBytes = sdkInfo.outOmmArrayHistogramSizeInBytes;
  out_info.outOmmIndexHistogramSizeInBytes = sdkInfo.outOmmIndexHistogramSizeInBytes;
  out_info.outOmmPostDispatchInfoSizeInBytes = sdkInfo.outOmmPostDispatchInfoSizeInBytes;
  out_info.numTransientPoolBuffers = sdkInfo.numTransientPoolBuffers;
  for (uint32_t i = 0; i < MAX_TRANSIENT_POOL_BUFFERS; ++i)
    out_info.transientPoolBufferSizeInBytes[i] = sdkInfo.transientPoolBufferSizeInBytes[i];

  // Validate the SDK sizes against a per-bake cap before allocate_pending_bake consumes them. The
  // histogram/desc/post-dispatch buffers are each allocated twice (device + readback). We reject
  // rather than clamp: the SDK needs exactly these sizes, so a smaller allocation would corrupt output.
  uint64_t totalBytes = uint64_t(out_info.outOmmArraySizeInBytes) + out_info.outOmmDescSizeInBytes +
                        out_info.outOmmIndexBufferSizeInBytes + 2ull * out_info.outOmmArrayHistogramSizeInBytes +
                        2ull * out_info.outOmmIndexHistogramSizeInBytes + 2ull * out_info.outOmmPostDispatchInfoSizeInBytes;
  for (uint32_t i = 0; i < out_info.numTransientPoolBuffers && i < MAX_TRANSIENT_POOL_BUFFERS; ++i)
    totalBytes += out_info.transientPoolBufferSizeInBytes[i];

  // Capture tools (PIX, RenderDoc, ...) pad GPU allocations with debug data, which inflates the
  // SDK-reported bake sizes past the normal cap. Double the cap while one is attached so OMM baking
  // still works under a capture. Queried once: a capture tool stays loaded for the whole session.
  static const uint64_t maxBakeAllocationBytes =
    OMM_MAX_BAKE_ALLOCATION_BYTES * (d3d::driver_command(Drv3dCommand::IS_ANY_CAPTURE_TOOL_LOADED) ? 2 : 1);

  if (totalBytes > maxBakeAllocationBytes)
  {
    const char *alphaTexName = input.alphaTexture ? input.alphaTexture->getTexName() : nullptr;
    logerr("omm: pre-dispatch info requests %llu bytes for one bake (alpha texture <%s>), over the %llu cap; rejecting the "
           "bake. Most likely an asset issue (alpha-tested mesh with too many opaque triangles) -- contact an artist.",
      totalBytes, alphaTexName ? alphaTexName : "<unknown>", maxBakeAllocationBytes);
    return false;
  }

  return true;
}

static bool allocate_pending_bake(PendingBake &allocated, const char *name_prefix)
{
  const char *prefix = name_prefix ? name_prefix : "omm";
  const unsigned arrayDataFlags = SBCF_UA_SR_BYTE_ADDRESS | SBCF_OPACITY_MICRO_MAP_TRIANGLE_SOURCE_DATA;
  const unsigned outputFlags = SBCF_UA_SR_BYTE_ADDRESS;
  const unsigned transientFlags = SBCF_UA_SR_BYTE_ADDRESS | SBCF_MISC_DRAWINDIRECT;

  if (!create_buffer(allocated.outOmmArrayData, allocated.outOmmArraySizeInBytes, arrayDataFlags,
        String(0, "%s_out_array_data", prefix).c_str(), OMM_RESOURCE_TAG) ||
      !create_buffer(allocated.outOmmDescArray, allocated.outOmmDescSizeInBytes, outputFlags,
        String(0, "%s_out_desc_array", prefix).c_str(), OMM_RESOURCE_TAG) ||
      !create_buffer(allocated.outOmmDescArrayHistogram, allocated.outOmmArrayHistogramSizeInBytes, outputFlags,
        String(0, "%s_out_desc_histogram", prefix).c_str(), OMM_RESOURCE_TAG) ||
      !create_buffer(allocated.outOmmIndexBuffer, allocated.outOmmIndexBufferSizeInBytes, outputFlags,
        String(0, "%s_out_index_buffer", prefix).c_str(), OMM_RESOURCE_TAG) ||
      !create_buffer(allocated.outOmmIndexHistogram, allocated.outOmmIndexHistogramSizeInBytes, outputFlags,
        String(0, "%s_out_index_histogram", prefix).c_str(), OMM_RESOURCE_TAG) ||
      !create_buffer(allocated.outPostDispatchInfo, allocated.outOmmPostDispatchInfoSizeInBytes, outputFlags,
        String(0, "%s_out_post_dispatch_info", prefix).c_str(), OMM_RESOURCE_TAG))
    return false;

  if (!create_readback_buffer(allocated.readbackOmmDescArrayHistogram, allocated.outOmmArrayHistogramSizeInBytes,
        String(0, "%s_readback_desc_histogram", prefix).c_str(), OMM_RESOURCE_TAG) ||
      !create_readback_buffer(allocated.readbackOmmIndexHistogram, allocated.outOmmIndexHistogramSizeInBytes,
        String(0, "%s_readback_index_histogram", prefix).c_str(), OMM_RESOURCE_TAG) ||
      !create_readback_buffer(allocated.readbackPostDispatchInfo, allocated.outOmmPostDispatchInfoSizeInBytes,
        String(0, "%s_readback_post_dispatch_info", prefix).c_str(), OMM_RESOURCE_TAG))
    return false;

  for (uint32_t i = 0; i < allocated.numTransientPoolBuffers; ++i)
  {
    if (
      i >= MAX_TRANSIENT_POOL_BUFFERS || !create_buffer(allocated.transientPoolBuffers[i], allocated.transientPoolBufferSizeInBytes[i],
                                           transientFlags, String(0, "%s_transient_%u", prefix, i).c_str(), OMM_RESOURCE_TAG))
      return false;
  }

  return true;
}

static bool dispatch_internal(Context &ctx, PendingBake &bake, const BakeInput &input, const Resources &resources)
{
  TIME_D3D_PROFILE(omm_bake_dispatch);

  if (!ctx.pipeline || !ctx.pipelineInfo)
    return false;

  omm_uv_cutout_lines_var.set_float4(input.uvCutout.lines.x, input.uvCutout.lines.y, input.uvCutout.lines.z, input.uvCutout.lines.w);
  omm_uv_cutout_enabled_var.set_int(input.uvCutout.enabled ? 1 : 0);

  ommGpuDispatchConfigDesc sdkConfig;
  if (!to_sdk(input, sdkConfig))
    return false;

  const ommGpuDispatchChain *chain = nullptr;
  if (!sdk_ok(ctx, ommGpuDispatch(static_cast<ommGpuPipeline>(ctx.pipeline), &sdkConfig, &chain), "Gpu::Dispatch") || !chain)
    return false;

  const d3d::SamplerHandle sampler = request_sampler(sdkConfig.runtimeSamplerDesc);
  BarrierTracker barriers;
  for (uint32_t i = 0; i < chain->numDispatches; ++i)
  {
    const ommGpuDispatchDesc &desc = chain->dispatches[i];
    switch (desc.type)
    {
      case ommGpuDispatchType_Compute:
        if (!execute_compute(ctx, bake, barriers, resources, *chain, desc.compute, sampler))
          return false;
        break;
      case ommGpuDispatchType_ComputeIndirect:
        if (!execute_compute_indirect(ctx, bake, barriers, resources, *chain, desc.computeIndirect, sampler))
          return false;
        break;
      case ommGpuDispatchType_DrawIndexedIndirect: logerr("omm: graphics dispatch is not supported"); return false;
      case ommGpuDispatchType_BeginLabel:
      case ommGpuDispatchType_EndLabel: break;
      default: return false;
    }
  }

  barriers.flush_all();
  return true;
}

bool has_free_bake_slot(const Context &ctx)
{
  for (const PendingBake &bake : ctx.pendingBakes)
    if (bake.state == PendingBakeState::Free)
      return true;
  return false;
}

bool begin_bake(Context &ctx, const BakeInput &input, BakeHandle &out_handle)
{
  TIME_D3D_PROFILE(omm_bake);

  out_handle = {};
  if (!ctx.pipeline || !ctx.pipelineInfo)
    return false;

  uint32_t slot = MAX_PENDING_BAKES;
  for (uint32_t i = 0; i < MAX_PENDING_BAKES; ++i)
    if (ctx.pendingBakes[i].state == PendingBakeState::Free)
    {
      slot = i;
      break;
    }

  if (slot == MAX_PENDING_BAKES)
  {
    logerr("omm: no free pending bake slots");
    return false;
  }

  PendingBake &bake = ctx.pendingBakes[slot];
  close_pending_bake(bake);
  bake.generation++;
  if (bake.generation == 0)
    bake.generation = 1;

  if (!fill_pre_dispatch_info(ctx, input, bake) ||
      !allocate_pending_bake(bake, String(0, "omm_bake_%u_%u", slot, bake.generation).c_str()))
  {
    close_pending_bake(bake);
    return false;
  }

  const Resources resources = make_resources(input, bake);
  if (!clear_omm_array_data(bake) || !dispatch_internal(ctx, bake, input, resources) || !issue_readbacks(bake))
  {
    close_pending_bake(bake);
    return false;
  }

  bake.state = PendingBakeState::Dispatched;
  out_handle.slot = slot;
  out_handle.generation = bake.generation;
  return true;
}

bool is_bake_ready(Context &ctx, BakeHandle handle)
{
  PendingBake *bake = get_pending_bake(ctx, handle);
  if (!bake)
    return false;

  if (bake->state == PendingBakeState::Ready)
    return true;

  if (!bake->readbackQuery || !d3d::get_event_query_status(bake->readbackQuery.get(), false))
    return false;

  bake->state = PendingBakeState::Ready;
  return true;
}

void clear_result(BakeResult &result)
{
  debug_unregister_bake_result(result);
  result = {};
}

ConsumeBakeResult consume_bake(Context &ctx, BakeHandle handle, BakeResult &out_result, BakeStats *out_stats)
{
  if (!is_bake_ready(ctx, handle))
    return ConsumeBakeResult::NotReady;

  PendingBake *bake = get_pending_bake(ctx, handle);
  if (!bake)
    return ConsumeBakeResult::Failed;

  PostDispatchInfo postInfo;
  if (!read_post_dispatch_info(bake->readbackPostDispatchInfo.getBuf(), postInfo))
  {
    close_pending_bake(*bake);
    return ConsumeBakeResult::Failed;
  }

  clear_result(out_result);
  if (!convert_histogram(bake->readbackOmmDescArrayHistogram.getBuf(), bake->outOmmArrayHistogramSizeInBytes,
        out_result.arrayBuildDescs) ||
      !convert_histogram(bake->readbackOmmIndexHistogram.getBuf(), bake->outOmmIndexHistogramSizeInBytes, out_result.blasLinkageDescs))
  {
    close_pending_bake(*bake);
    return ConsumeBakeResult::Failed;
  }

  out_result.indexFormat = bake->outOmmIndexBufferFormat;
  out_result.indexCount = bake->outOmmIndexCount;
  out_result.arrayDataSizeInBytes = postInfo.outOmmArraySizeInBytes ? postInfo.outOmmArraySizeInBytes : bake->outOmmArraySizeInBytes;
  out_result.descArraySizeInBytes = postInfo.outOmmDescSizeInBytes ? postInfo.outOmmDescSizeInBytes : bake->outOmmDescSizeInBytes;
  out_result.indexBufferSizeInBytes = bake->outOmmIndexBufferSizeInBytes;
  out_result.arrayData = eastl::move(bake->outOmmArrayData);
  out_result.descArray = eastl::move(bake->outOmmDescArray);
  out_result.indexBuffer = eastl::move(bake->outOmmIndexBuffer);

  if (out_stats)
    *out_stats = make_bake_stats(postInfo);

  close_pending_bake(*bake);
  return ConsumeBakeResult::Ready;
}

bool wait_bake(Context &ctx, BakeHandle handle, BakeResult &out_result, BakeStats *out_stats)
{
  PendingBake *bake = get_pending_bake(ctx, handle);
  if (!bake || !bake->readbackQuery || !d3d::get_event_query_status(bake->readbackQuery.get(), true))
    return false;

  bake->state = PendingBakeState::Ready;
  return consume_bake(ctx, handle, out_result, out_stats) == ConsumeBakeResult::Ready;
}

void discard_bake(Context &ctx, BakeHandle handle)
{
  if (PendingBake *bake = get_pending_bake(ctx, handle))
    close_pending_bake(*bake);
}

raytrace::OpacityMicroMapTriangleArrayBuildInfo make_array_build_info(const BakeResult &result, Sbuffer *scratch,
  uint32_t scratch_offset, uint32_t scratch_size, RaytraceBuildFlags flags)
{
  raytrace::OpacityMicroMapTriangleArrayBuildInfo info;
  info.ommDesc = result.arrayBuildDescs;
  info.flags = flags;
  info.inputBuffer = result.arrayData.getBuf();
  info.perOpacityMicroMapDescriptions = result.descArray.getBuf();
  info.perOpacityMicroMapDescriptionsStride = sizeof(raytrace::InBufferOpacityMicroMapDescription);
  info.scratchSpaceBuffer = scratch;
  info.scratchSpaceBufferOffsetInBytes = scratch_offset;
  info.scratchSpaceBufferSizeInBytes = scratch_size;
  return info;
}

RaytraceGeometryDescription::OpacityMicroMapLinkage make_geometry_linkage(const BakeResult &result,
  RaytraceOpacityMicroMapTriangleArray *triangle_array)
{
  return {
    .indexBuffer = result.indexBuffer.getBuf(),
    .indexFormat = to_raytrace_index_format(result.indexFormat),
    .indexBufferOffsetInIndexUnits = 0,
    .indexBufferStrideInIndexUnits = 1,
    .triangleArrayOffset = 0,
    .triangleArray = triangle_array,
    .ommDesc = result.blasLinkageDescs,
  };
}

} // namespace render::omm
